/* change the strings "%63s" etc too! */
#define HISTSIZE 64

struct Oss {
	int   room;
	int   points;
	int   friends;
	int   enemies;
	int   education;
	int   trouble;
	short flags;
	char  history[HISTSIZE];
	char  name[8];
};

void OssMessage(const struct Oss *o, const char *message, ...);
