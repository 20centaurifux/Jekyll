#!/bin/sh
export G_SLICE=always-malloc && export G_DEBUG=gc-friendly && valgrind --tool=memcheck --leak-check=full --leak-resolution=high --num-callers=20 --log-file=vgdump.log ./jekyll
