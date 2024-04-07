
struct node {
    struct node *next;
    void *data;
};

struct node *node_create(int size);

void node_destroy(struct node *node);

int list_get_length(struct node *list);

int list_push(struct node *list, struct node *node);

int list_pop(struct node *list, struct node **node);

