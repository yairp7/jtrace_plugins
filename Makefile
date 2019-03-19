#
# If you need help with this - you can find me at NewAndroidBook.com
#
FILES=config.c jtrace.c syscall_handlers.c

CC_arm=${NDK_HOME}/android-21-arm-toolchain/bin/arm-linux-androideabi-clang

x64:
	gcc $(FILES) -DGNU_SOURCE -DX86 -ldl -lpthread -o jtrace.x86 -g2 -Wall -Wl,--dynamic-list=exports

x32:
	gcc $(FILES) -DX86 -ldl -lpthread -o jtrace.x32 -g2 -Wall -m32

arm: arm64 arm32

arm32:
	./androidcc

arm64:
	./androidcc64

dist:
	tar zcvf jtrace.tgz arm32/ arm64/ x86/ jtraceAPI.h Plugins/ Makefile androidcc androidcc64 androidcc64.plugin

plugin-arm32:
	$(CC_arm) Plugins/jtrace_plugins/fileAccessLogger.c -c -fPIC -o arm32/fileAccessLogger.o
	$(CC_arm) -shared arm32/fileAccessLogger.o -o arm32/fileAccessLogger.so
	rm arm32/fileAccessLogger.o

plugin-x86:
	cc Plugins/jtrace_plugins/fileAccessLogger.c -c -fPIC -o x86/fileAccessLogger.o
	ld -shared x86/fileAccessLogger.o -o x86/fileAccessLogger.so

plugin.arm64:
	./androidcc64.plugin
