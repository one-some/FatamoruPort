#include "fs.h"
#include "rosetta.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>

char* find_file(char* storage, const char* patterns[]) {
    for (int i=0; patterns[i]; i++) {

        char path_buffer[256];
        snprintf(path_buffer, sizeof(path_buffer), patterns[i], storage);

        FILE *fp = fopen(path_buffer, "r");
        if (fp) {
            fclose(fp);
            return strdup(path_buffer);
        }
    }

	printf("[file] Couldn't find '%s' anywhere:\n", storage);
    for (int i=0; patterns[i]; i++) {
		printf("- %s\n", patterns[i]);
	}

    assert(false);
	return NULL;
}

char* find_bgm(char* storage) {
	const char* patterns[] = { DATA_PATH("bgm/%s.ogg"), NULL };
	return find_file(storage, patterns);
}

char* find_sfx(char* storage) {
	const char* patterns[] = { DATA_PATH("sound/%s.ogg"), NULL };
	return find_file(storage, patterns);
}

char* find_image(char* storage) {
	const char* patterns[] = {
		// HACK: Include port paths. Also include them first so it's faster
		DATA_PATH("bgimage/%s.t3x"),
		DATA_PATH("image/%s.t3x"),
		DATA_PATH("fgimage/%s.t3x"),

		DATA_PATH("bgimage/%s.png"),
		DATA_PATH("bgimage/%s.jpg"),
		DATA_PATH("image/%s.png"),
		DATA_PATH("image/%s.jpg"),
		DATA_PATH("fgimage/%s.png"),
		DATA_PATH("fgimage/%s.jpg"),
		NULL
	};
	return find_file(storage, patterns);
}

char* find_script(char* storage) {
	const char* patterns[] = {
		DATA_PATH("scenario/%s"),
		DATA_PATH("others/%s"),
		DATA_PATH("system/%s"),
		DATA_PATH("static/%s"),
		NULL
	};
	return find_file(storage, patterns);
}

char* find_font(char* storage) {
	const char* patterns[] = {
		PATH("static/%s.bcfnt"),
		PATH("static/%s.ttf"),
		NULL
	};
	return find_file(storage, patterns);
}
