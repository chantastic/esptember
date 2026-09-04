# Lesson media sources

These are saved source assets for the ESPtember media lessons.
`python3 scripts/make-media.py` converts these local files with FFmpeg; regeneration does not depend on a live profile URL staying unchanged.
Retrieved September 4, 2026.

| Local source | Origin | Used in |
| --- | --- | --- |
| `chan-github.jpg` | [chantastic on GitHub](https://github.com/chantastic), [avatar image](https://avatars.githubusercontent.com/u/658360?v=4) | Day 03; first gallery slide |
| `chan-react-conf.jpg` | [React Conf speakers](https://conf.react.dev/speakers), image labeled Michael Chan; [image](https://cdn.sessionize.com/image/fa1c-400o400o1-pRHcKFFUQPd3j27vtC87rT.jpg) | Second gallery slide |
| `chan-react-advanced.jpg` | [React Advanced 2022](https://reactadvanced.com/2022/), Michael Chan's speaker entry links to `github.com/chantastic`; [image](https://media.graphassets.com/UPyaMi3ATvifJyG6BwQ0) | Third gallery slide |
| `homer-bushes-original.gif` | [Scared Homer Simpson on GIPHY](https://giphy.com/gifs/the-simpsons-scared-homer-simpson-jUwpNzg9IcyrK), uploaded by reactionseditor; [GIF](https://media4.giphy.com/media/jUwpNzg9IcyrK/giphy.gif) | Day 05 |
| `big-buck-bunny-clip.mp4` | [Blender Foundation's official download](https://download.blender.org/peach/bigbuckbunny_movies/BigBuckBunny_320x180.mp4.zip), 01:00–01:03 excerpt | Day 06 |

The portraits are public profile/speaker images of Michael Chan (chantastic), selected for this project at his request.
The Homer excerpt comes from *The Simpsons*; its inclusion does not assign an open-source license to that media.
Source media retain their respective copyrights.

*Big Buck Bunny*: © 2008, Blender Foundation / [www.bigbuckbunny.org](https://www.bigbuckbunny.org/).
Licensed under [Creative Commons Attribution 3.0](https://creativecommons.org/licenses/by/3.0/), as stated on the project's [about page](https://peach.blender.org/about/).
The saved excerpt is trimmed and re-encoded without audio; the firmware version is additionally resized, padded, and converted to RGB565 at 10 fps.

## Conversions

- Portraits: center-cropped to 240 × 240 and converted to little-endian RGB565, 115,200 bytes each.
- Homer: scaled from 500 × 375 to cover 368 × 448, then cropped horizontally with a 26-pixel leftward picture shift (7% of the panel width) and re-encoded as a GIF with a 128-color palette, no added dithering, and 100 ms frame intervals; all frames are complete and opaque (`reserve_transparent=0`, `-gifflags 0`). The firmware decodes into RGB565; no alpha channel is needed. Inspect the converted duration rather than assuming the original timing survives quantization exactly.
- Big Buck Bunny: three-second clip, padded to 368 × 224, 10 fps, little-endian RGB565; the display uses its native size. The stored file remains 4,945,920 bytes.

To recreate the saved movie excerpt after unzipping the official download:

```sh
ffmpeg -ss 60 -i BigBuckBunny_320x180.mp4 -t 3 -an \
  -c:v libx264 -crf 18 -pix_fmt yuv420p big-buck-bunny-clip.mp4
```

`SHA256SUMS` records the saved source files, not the remotely hosted originals at any future date.
