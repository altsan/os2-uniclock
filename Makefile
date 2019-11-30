# IBM C/C++ (VisualAge) Makefile for UNICLOCK.  Version 3.65 recommended.
# Also requires MAKEDESC.CMD in path.
#
# Uncomment (or set via command line) to enable PMPRINTF debugging
# (requires PMPRINTF, obviously).
# PMPF = 1

!ifndef NLV
    NLV = 001
!endif

BL_NAME = "Universal Clock for OS/2 (NLV $(NLV))"
BL_VEND = "Alex Taylor"

CC     = icc.exe
RC     = rc.exe
LINK   = ilink.exe
IPFC   = ipfc.exe
CFLAGS = /Gm /Ss /Q+ /Wuse      # /Wrea /Wuni
RFLAGS = -n -cc $(NLV)
LFLAGS = /NOE /PMTYPE:PM /NOLOGO
NAME   = uclock
OBJS   = $(NAME).obj cfg_prog.obj cfg_clk.obj wtdpanel.obj sunriset.obj
LIBS   = libconv.lib libuls.lib
ICONS  = program.ico up.ico down.ico

!ifdef PMPF
    LIBS   = $(LIBS) pmprintf.lib
    CFLAGS = $(CFLAGS) /D_PMPRINTF_
!endif

!ifdef DEBUG
    CFLAGS   = $(CFLAGS) /Ti /Tm
    LFLAGS   = $(LFLAGS) /DEBUG
!endif

!if "$(NLV)" == "081"
    RFLAGS = $(RFLAGS) -cp 932
!elif "$(NLV)" == "082"
    RFLAGS = $(RFLAGS) -cp 949
!elif "$(NLV)" == "086"
    RFLAGS = $(RFLAGS) -cp 1386
!elif "$(NLV)" == "088"
    RFLAGS = $(RFLAGS) -cp 950
!endif

all         : $(NAME).exe $(NAME).hlp

wtdpanel.obj: wtdpanel.c wtdpanel.h ids.h Makefile

uclock.obj  : uclock.c $(NAME).h wtdpanel.h ids.h Makefile

cfg_prog.obj: cfg_prog.c $(NAME).h wtdpanel.h ids.h Makefile

cfg_clk.obj:  cfg_clk.c $(NAME).h wtdpanel.h ids.h Makefile

$(NAME).exe : $(OBJS) $(NAME).h ids.h wtdpanel.h $(NAME).res Makefile
                -touch $(NAME).def
                -makedesc -D$(BL_NAME) -N$(BL_VEND) -V"^#define=SZ_VERSION,uclock.h" $(NAME).def
                $(LINK) $(LFLAGS) $(OBJS) $(LIBS) $(NAME).def
                $(RC) -x -n $(NAME).res $@

$(NAME).res : {$(NLV)}$(NAME).rc {$(NLV)}$(NAME).dlg ids.h $(ICONS)
                %cd $(NLV)
                $(RC) -i .. -r -n $(RFLAGS) $(NAME).rc ..\$@
                %cd ..

$(NAME).hlp : {$(NLV)}$(NAME).ipf
                %cd $(NLV)
                $(IPFC) -d:$(NLV) $(NAME).ipf ..\$@
                %cd ..

clean       :
                -del $(OBJS) $(NAME).res $(NAME).exe $(NAME).hlp 2>NUL

