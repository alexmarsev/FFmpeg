include $(SUBDIR)../config.mak

NAME = swscale
FFLIBS = avutil

HEADERS = swscale.h                                                     \
          version.h                                                     \

OBJS = input.o                                          \
       options.o                                        \
       output.o                                         \
       rgb2rgb.o                                        \
       swscale.o                                        \
       swscale_unscaled.o                               \
       utils.o                                          \
       yuv2rgb.o                                        \

# Windows resource file
SLIBOBJS-$(HAVE_GNU_WINDRES) += swscaleres.o

TESTPROGS = colorspace                                                  \
            swscale
