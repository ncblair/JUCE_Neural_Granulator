% Journal

# 2022 February 16
- Grain class refactor
- Grain playback 
## Grain Playback

Old style: The audio buffer referenced by a grain dynamically resamples if its playback speed changed (e.g. pitch bending). Each voice has a `pitch_grain_buffer` that stores the pitched version of the original buffer.

New style: Each time the playback speed changes, the grain plays through the original buffer at a different rate. We do ~math~ for interpolation. This avoids dynamic resampling of entire audio files as well as extra storage per voice.

