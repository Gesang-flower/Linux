#ifndef DATABASE_CTR_H
#define DATABASE_CTR_H

int create_table(const char *data_base_name);
int database_insert(const char *data_base_name, const char *data);
int database_select(const char *data_base_name, char *response_message, int max_size);

#endif
