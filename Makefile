# IBM C/C++ (VisualAge) Makefile for UNICLOCK, English version.
#
#
#PMPF = 1

BL_NAME = "Universal Clock for OS/2"
BL_VEND = "Alex Taylor"

CC     = icc.exe
RC     = rc.exe
LINK   = ilink.exe
IPFC   = ipfc.exe
CFLAGS = /Gm /Ss /Q+ /Wuse      # /Wrea /Wuni
RFLAGS = -x -n
LFLAGS = /NOE /PMTYPE:PM /NOLOGO
NAME   = uclock
OBJS   = $(NAME).obj wtdpanel.obj sunriset.obj
LIBS   = libconv.lib libuls.lib

!ifdef PMPF
    LIBS   = $(LIBS) pmprintf.lib
    CFLAGS = $(CFLAGS) /D_PMPRINTF_
!endif

!ifdef DEBUG
    CFLAGS   = $(CFLAGS) /Ti /Tm
    LFLAGS   = $(LFLAGS) /DEBUG
!endif

all         : $(NAME).exe

$(NAME).exe : $(OBJS) $(NAME).h ids.h $(NAME).res Makefile
                -touch $(NAME).def
                -makedesc -D$(BL_NAME) -N$(BL_VEND) -V"^#define=SZ_VERSION,uclock.h" $(NAME).def
                $(LINK) $(LFLAGS) $(OBJS) $(LIBS) $(NAME).def
                $(RC) $(RFLAGS) $(NAME).res $@

$(NAME).res : $(NAME).rc $(NAME).dlg ids.h program.ico
                $(RC) -r -n $(NAME).rc $@

wtdpanel.obj: wtdpanel.c uclock.h ids.h

uclock.obj  : uclock.c uclock.h ids.h

clean       :
                -del $(OBJS) $(NAME).res $(NAME).exe 2>NUL

