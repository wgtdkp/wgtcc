#include <stdio.h>
#include <stdlib.h>

typedef char Node;

static Node *ast_gvar(char *ty, char *name) {
    //Node *r = make_ast(&(Node){ AST_GVAR, ty, .varname = name, .glabel = name });
    printf("%s\n", name);
    printf("2\n");
    fflush(stdout);
    //map_put(globalenv, name, r);
    return 0;
}

static void define_builtin(char *name, char *rettype, char *paramtypes) {
    printf("%s\n", name);
    ast_gvar(0, name);
}

int main() {
  define_builtin("__builtin_return_address", 0, 0);
  return 0;
}
