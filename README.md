# Videostream on secondary monitor
## Dependencies
<ul>
  <li>Raspbian installation (with graphical environment)</li>
  <li>mpv</li>
</ul>

## Command
========================
    mpv --untimed --profile=low-latency --speed=100.0 --video-latency-hacks=yes --framedrop=no --opengl-glfinish=yes --opengl-swapinterval=0 --no-audio --stop-screensaver --fs --panscan=1.0 --pause=no /dev/video0

Note: Add this line to ~/.xsessionrc to have it start on login
