# IBM C/C++ (VisualAge) Makefile for UNICLOCK, English version.
#
#

CC     = icc.exe
RC     = rc.exe
LINK   = ilink.exe
IPFC   = ipfc.exe
CFLAGS = /Gm /Ss /Q+ /Wuse      # /Wrea /Wuni
RFLAGS = -x -n
LFLAGS = /NOE /PMTYPE:PM /NOLOGO
NAME   = uclock
OBJS   = $(NAME).obj wtdpanel.obj
LIBS   = libconv.lib libuls.lib

!ifdef DEBUG
    CFLAGS   = $(CFLAGS) /Ti /Tm
    LFLAGS   = $(LFLAGS) /DEBUG
!endif

all         : $(NAME).exe

$(NAME).exe : $(OBJS) $(NAME).h ids.h $(NAME).res
                $(LINK) $(LFLAGS) $(OBJS) $(LIBS)
                $(RC) $(RFLAGS) $(NAME).res $@

$(NAME).res : $(NAME).rc $(NAME).dlg ids.h program.ico
                $(RC) -r -n $(NAME).rc $@

clean       :
                -del $(OBJS) $(NAME).res $(NAME).exe 2>NUL

