const PEAK_EVERY_BLOCKS = 8;

class EngineProcessor extends AudioWorkletProcessor {
  constructor({ processorOptions: { wasm } }) {
    super();

    try {
      // WASI imports are never called
      const stub = new Proxy({}, { get: () => () => 0 });
      const instance = new WebAssembly.Instance(new WebAssembly.Module(wasm), new Proxy({}, { get: () => stub }));
      this.engine = instance.exports;
      this.engine._initialize?.();
      this.engine.prepare(sampleRate);
    } catch (error) {
      this.engine = null;
      this.port.postMessage({ type: 'error', message: String(error) });
      return;
    }

    this.blocks = 0;

    this.port.onmessage = ({ data }) => {
      if (data.type === 'param') this.engine.setParam(data.index, data.value);
      else if (data.type === 'midi') this.engine.midi(data.bytes[0], data.bytes[1], data.bytes[2]);
      else if (data.type === 'transport') this.engine.setTransport(data.bpm, data.playing, data.ppq);
    };

    this.port.postMessage({ type: 'ready' });
  }

  process(_inputs, [output]) {
    if (!this.engine) return false;

    const frames = output[0].length;
    const pointer = this.engine.render(frames);

    // Only changes if memory grows
    if (this.view?.buffer !== this.engine.memory.buffer || this.view.byteOffset !== pointer || this.view.length !== frames) {
      this.view = new Float32Array(this.engine.memory.buffer, pointer, frames);
    }

    for (const channel of output) channel.set(this.view);

    // The page's meter falls on its own, so silence needs no message
    if (++this.blocks % PEAK_EVERY_BLOCKS === 0) {
      const peak = this.engine.peak();
      if (peak > 0) this.port.postMessage({ type: 'level', peaks: [peak] });
    }

    return true;
  }
}

registerProcessor('engine-processor', EngineProcessor);
