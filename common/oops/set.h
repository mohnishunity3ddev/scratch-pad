#ifndef SET_H
#define SET_H

extern const void *__Set;
void *add(void *set, const void *element);
void *find(const void *set, const void *element);
void *drop(void *set, const void *element);
int contains(const void *set, const void *element);

#endif