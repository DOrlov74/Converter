# Converter

Converter is a simple gstreamer pipeline console application, based on [Gstreamer Basic tutorials](https://gstreamer.freedesktop.org/documentation/tutorials/basic/index.html?gi-language=c).
The program fetches a media file from the command argument and convert it to new matroska files with video and audio. File for testing the program could be downloaded from [Link](https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm)

## Command example

Converter.exe sintel_trailer-480p.webm

## Pipeline example

gst-launch-1.0 souphttpsrc location=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm ! matroskademux name=d d.video_0 ! queue ! matroskamux ! filesink location=sintel_video.mkv d.audio_0 ! queue ! vorbisparse ! matroskamux ! filesink location=sintel_audio.mka
