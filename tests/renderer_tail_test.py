import pathlib
import struct
import subprocess
import sys
import tempfile
import wave


def decode_int24_little_endian(sample: bytes) -> int:
    unsigned = int.from_bytes(sample, byteorder="little", signed=False)
    return unsigned - (1 << 24) if unsigned & (1 << 23) else unsigned


def main() -> int:
    if len(sys.argv) != 2:
        raise RuntimeError("renderer executable path is required")

    renderer = pathlib.Path(sys.argv[1]).resolve()
    if not renderer.is_file():
        raise RuntimeError(f"renderer not found: {renderer}")

    frame_count = 2500  # Deliberately not divisible by the renderer's 1024-sample block.
    with tempfile.TemporaryDirectory(prefix="rapready-render-test-") as temporary:
        temporary_path = pathlib.Path(temporary)
        source = temporary_path / "tail-impulse.wav"
        rendered = temporary_path / "tail-impulse-rendered.wav"

        with wave.open(str(source), "wb") as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(48000)
            output.writeframes(b"\x00\x00" * (frame_count - 1) + struct.pack("<h", 20000))

        completed = subprocess.run(
            [str(renderer), str(source), str(rendered), "0"],
            check=False,
            capture_output=True,
            text=True,
            timeout=30,
        )
        if completed.returncode != 0:
            raise RuntimeError(f"renderer failed: {completed.stdout}\n{completed.stderr}")

        with wave.open(str(rendered), "rb") as result:
            if result.getnchannels() != 1 or result.getsampwidth() != 3:
                raise RuntimeError("renderer did not produce mono 24-bit PCM")
            if result.getnframes() != frame_count:
                raise RuntimeError(f"expected {frame_count} frames, got {result.getnframes()}")
            audio = result.readframes(frame_count)

        samples = [decode_int24_little_endian(audio[index : index + 3]) for index in range(0, len(audio), 3)]
        if max(abs(value) for value in samples[:-1]) > 1:
            raise RuntimeError("latency compensation moved the final impulse earlier")
        if abs(samples[-1]) < 1_000_000:
            raise RuntimeError("final partial-block impulse was lost")

    print("PASS  offline renderer preserves a final-sample impulse and exact duration")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

