/* Listagem de pasta sem malloc (opendir aloca e derruba o app neste console). */
#pragma once

typedef void (*DirCb)(const char* name, int is_dir, void* ud);

/* 0 = ok; -1 = pasta inexistente ou listagem indisponivel neste sistema */
int dir_each(const char* path, DirCb cb, void* ud);
