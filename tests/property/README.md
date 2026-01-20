# Property-Based Tests

This directory contains property-based tests for the Kick Drum Synthesizer using fast-check.

## Setup

Install dependencies:
```bash
npm install
```

## Running Tests

Run all property tests:
```bash
npm test
```

Run tests in watch mode:
```bash
npm run test:watch
```

Run tests with coverage:
```bash
npm run test:coverage
```

## Test Structure

Property tests are organized by feature:
- `generators/` - Tests for oscillators and noise generators
- `modulation/` - Tests for ring modulation and mixing
- `envelopes/` - Tests for envelope system
- `voice/` - Tests for voice management
- `effects/` - Tests for compressor and reverb
- `parameters/` - Tests for parameter management
- `presets/` - Tests for preset serialization
- `midi/` - Tests for MIDI handling

## Property Test Format

Each property test should:
1. Reference the design document property number
2. Run minimum 100 iterations
3. Include a comment tag: `// Feature: kick-drum-synthesizer, Property X: [Name]`
4. Validate the corresponding requirement(s)

Example:
```typescript
// Feature: kick-drum-synthesizer, Property 1: Ring Modulation Multiplication
// Validates: Requirements 1.5, 1.6
test('ring modulation at 100% depth equals carrier × modulator', () => {
  fc.assert(
    fc.property(
      fc.float({ min: -1, max: 1 }),
      fc.float({ min: -1, max: 1 }),
      (carrier, modulator) => {
        const ringMod = new RingModulator(1.0); // 100% depth
        const output = ringMod.process(carrier, modulator);
        expect(output).toBeCloseTo(carrier * modulator, 5);
      }
    ),
    { numRuns: 100 }
  );
});
```
