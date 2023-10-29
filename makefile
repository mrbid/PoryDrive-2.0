all:
	mkdir -p release
	cc main.c glad_gl.c -I inc -Ofast -lglfw -lm -o release/porydrive
	strip --strip-unneeded release/porydrive
	upx --lzma --best release/porydrive

test:
	cc main.c glad_gl.c -I inc -Ofast -lglfw -lm -o /tmp/porydrive_test
	/tmp/porydrive_test
	rm /tmp/porydrive_test

web:
	emcc main.c glad_gl.c -DWEB -O3 -s FILESYSTEM=0 -s USE_GLFW=3 -s ENVIRONMENT=web -s TOTAL_MEMORY=128MB -I inc -o bin/index.html --shell-file t.html

run:
	emrun bin/index.html

clean:
	rm -r release
