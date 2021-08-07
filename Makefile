# Makefile for libPegasusUPBv2Power and libPegasusUPBv2Focuser

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
CPPFLAGS = -std=c++17 -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
LDFLAGS = -shared -lstdc++
RM = rm -f
STRIP = strip
TARGET_LIBFocuser = libPegasusUPBv2Focuser.so
TARGET_LIBPower = libPegasusUPBv2Power.so

SRCS_Focuser = focuserMain.cpp pegasus_upbv2Focuser.cpp x2focuser.cpp x2powercontrol.cpp pegasus_upbv2.cpp
SRCS_Power = powerMain.cpp pegasus_upbv2.cpp x2powercontrol.cpp x2focuser.cpp pegasus_upbv2Focuser.cpp

OBJS_Focuser = $(SRCS_Focuser:.cpp=.o)
OBJS_Power = $(SRCS_Power:.cpp=.o)

.PHONY: all
all: ${TARGET_LIBFocuser} ${TARGET_LIBPower}
power : ${TARGET_LIBPower}
focuser : ${TARGET_LIBFocuser}

$(TARGET_LIBFocuser): $(OBJS_Focuser)
	$(CC) ${LDFLAGS} -o $@ $^
	$(STRIP) $@ >/dev/null 2>&1  || true

$(TARGET_LIBPower): $(OBJS_Power)
	$(CC) ${LDFLAGS} -o $@ $^
	$(STRIP) $@ >/dev/null 2>&1  || true

$(SRCS_Focuser:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

$(SRCS_Power:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

.PHONY: clean
clean:
	${RM} ${TARGET_LIBFocuser} ${TARGET_LIBPower} *.o *.d
