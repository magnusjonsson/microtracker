const struct synthdesc* finddesc(const char* name);

int synthdesc_instantiate(struct synthdesc const* synthdesc, double samplerate, void** state);
void synthdesc_deinstantiate(struct synthdesc const* synthdesc, void** state);
