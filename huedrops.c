#include <stdio.h>
#include <stdlib.h>

#define SYMBOLS_N 28
#define SYMBOL_WALL '.'
#define SYMBOL_WILD '*'
#define COLORS_MAX (SYMBOLS_N-2)

typedef struct group_s group_t;

typedef struct {
	int symbol;
	int free_groups_n;
	int rolls_n;
	int rolls_size;
	group_t *roll;
	group_t *merge;
}
color_t;

struct group_s {
	color_t *color;
	int rolled;
	int merged;
	int size;
	int rank;
	group_t *root;
	int links_n;
	group_t **links;
	group_t *last;
	group_t *next;
};

int read_tile(group_t *, int, int, int);
int find_color(int);
void add_color(int);
void set_color(color_t *, int);
void check_union(group_t *, group_t *);
int check_links(group_t *, group_t *);
int add_link(group_t *, group_t *);
void check_initial_roll(group_t *);
void set_group(group_t *, color_t *, int, int);
group_t *find_root(group_t *);
void huedrops(int);
int compare_colors(const void *, const void *);
void merge_color(int, color_t *);
void check_roll(group_t *, int);
void roll_group(group_t *, int);
void check_unroll(color_t *, int);
void unroll_group(group_t *);
void set_chain(group_t *);
void chain_group(group_t *, group_t *, group_t *);
void chain_group_last(group_t *, group_t *);
void chain_group_next(group_t *, group_t *);
void unchain_group(group_t *);
void free_groups(void);

int symbols[SYMBOLS_N] = { SYMBOL_WALL, SYMBOL_WILD, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' }, columns_n, tiles_n, colors_n, groups_n, *steps, steps_min, colors_stack_max, *colors_stack;
color_t colors[COLORS_MAX], color_wild = { SYMBOL_WILD, 0, 0, 0, NULL, NULL }, *color_target;
group_t *tiles, *tile_wild, **groups;

int main(void) {
	int rows_n, symbol, i;
	group_t *tile;

	/* Parse input, locate wild tile and group tiles */
	if (scanf("%d%d", &columns_n, &rows_n) != 2 || columns_n < 1 || rows_n < 1 || getchar() != '\n') {
		fprintf(stderr, "Invalid puzzle size\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	tiles_n = columns_n*rows_n;
	tiles = malloc(sizeof(group_t)*(size_t)(tiles_n+COLORS_MAX*2));
	if (!tiles) {
		fprintf(stderr, "Could not allocate memory for tiles\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	tile_wild = NULL;
	colors_n = 0;
	tile = tiles;
	for (i = 0; i < rows_n; i++) {
		int j;
		for (j = 0; j < columns_n-1; j++) {
			if (!read_tile(tile, i, j, ' ')) {
				free(tiles);
				return EXIT_FAILURE;
			}
			tile++;
		}
		if (!read_tile(tile, i, j, '\n')) {
			free(tiles);
			return EXIT_FAILURE;
		}
		tile++;
	}
	if (!tile_wild) {
		fprintf(stderr, "Wild tile not defined\n");
		fflush(stderr);
		free(tiles);
		return EXIT_FAILURE;
	}
	symbol = getchar();
	i = find_color(symbol);
	if (i == colors_n || getchar() != '\n') {
		fprintf(stderr, "Invalid target color\n");
		fflush(stderr);
		free(tiles);
		return EXIT_FAILURE;
	}
	color_target = colors+i;

	/* Make an inventory of all groups and add links between neighbours */
	groups = malloc(sizeof(group_t *)*(size_t)tiles_n);
	if (!groups) {
		fprintf(stderr, "Could not allocate memory for groups\n");
		fflush(stderr);
		free(tiles);
		return EXIT_FAILURE;
	}
	groups_n = 0;
	tile = tiles;
	for (i = 0; i < rows_n; i++) {
		int j;
		for (j = 0; j < columns_n; j++) {
			if (tile->color) {
				int k;
				group_t *group = find_root(tile);
				for (k = 0; k < groups_n && groups[k] != group; k++);
				if (k == groups_n) {
					groups[groups_n++] = group;
					if (!group->merged) {
						group->color->free_groups_n++;
					}
				}
				if (i > 0 && !check_links(find_root(tile-columns_n), group)) {
					free_groups();
					free(tiles);
					return EXIT_FAILURE;
				}
				if (j > 0 && !check_links(find_root(tile-1), group)) {
					free_groups();
					free(tiles);
					return EXIT_FAILURE;
				}
			}
			tile++;
		}
	}

	/* Roll groups linked to the wild tile then start the solver */
	for (i = 0; i < groups_n; i++) {
		check_initial_roll(groups[i]);
	}
	steps = malloc(sizeof(int)*(size_t)tiles_n);
	if (!steps) {
		fprintf(stderr, "Could not allocate memory for steps\n");
		fflush(stderr);
		free_groups();
		free(tiles);
		return EXIT_FAILURE;
	}
	steps_min = tiles_n;
	colors_stack_max = 0;
	huedrops(0);

	/* Free allocated memory then exit */
	if (colors_stack_max > 0) {
		free(colors_stack);
	}
	free(steps);
	free_groups();
	free(tiles);
	return EXIT_SUCCESS;
}

int read_tile(group_t *tile, int row, int column, int separator) {
	int symbol = getchar(), i;
	if (getchar() != separator) {
		fprintf(stderr, "Invalid separator\n");
		fflush(stderr);
		return 0;
	}
	for (i = 0; i < SYMBOLS_N && symbols[i] != symbol; i++);
	if (i == SYMBOLS_N) {
		fprintf(stderr, "Invalid symbol\n");
		fflush(stderr);
		return 0;
	}
	switch (symbol) {
		int j;
		case SYMBOL_WALL:
			set_group(tile, NULL, tiles_n, 0);
			break;
		case SYMBOL_WILD:
			if (tile_wild) {
				fprintf(stderr, "Wild tile already defined\n");
				fflush(stderr);
				return 0;
			}
			tile_wild = tile;
			set_group(tile, &color_wild, -1, 1);
			break;
		default:
			j = find_color(symbol);
			if (j == colors_n) {
				add_color(symbol);
			}
			set_group(tile, colors+j, tiles_n, 0);
			if (row > 0) {
				check_union(tile-columns_n, tile);
			}
			if (column > 0) {
				check_union(tile-1, tile);
			}
	}
	return 1;
}

int find_color(int symbol) {
	int i;
	for (i = 0; i < colors_n && colors[i].symbol != symbol; i++);
	return i;
}

void add_color(int symbol) {
	set_color(colors+colors_n, symbol);
	colors_n++;
}

void set_color(color_t *color, int symbol) {
	color->symbol = symbol;
	color->free_groups_n = 0;
	color->rolls_n = 0;
	color->rolls_size = 0;
	color->roll = tiles+tiles_n+colors_n;
	set_group(color->roll, NULL, tiles_n, 0);
	set_chain(color->roll);
	color->merge = tiles+tiles_n+COLORS_MAX+colors_n;
	set_group(color->merge, NULL, tiles_n, 0);
	set_chain(color->merge);
}

void check_union(group_t *group_a, group_t *group_b) {
	if (group_a->color == group_b->color) {
		group_t *root_a = find_root(group_a), *root_b = find_root(group_b);
		if (root_a != root_b) {
			if (root_a->rank < root_b->rank) {
				root_b->size += root_a->size;
				root_a->root = root_b;
			}
			else if (root_a->rank > root_b->rank) {
				root_a->size += root_b->size;
				root_b->root = root_a;
			}
			else {
				root_a->size += root_b->size;
				root_a->rank++;
				root_b->root = root_a;
			}
		}
	}
}

int check_links(group_t *group_a, group_t *group_b) {
	int i;
	if (!group_a->color || group_a == group_b) {
		return 1;
	}
	for (i = 0; i < group_a->links_n && group_a->links[i] != group_b; i++);
	if (i < group_a->links_n) {
		return 1;
	}
	return add_link(group_a, group_b) && add_link(group_b, group_a);
}

int add_link(group_t *from, group_t *to) {
	if (from->links_n == 0) {
		from->links = malloc(sizeof(group_t *));
		if (!from->links) {
			fprintf(stderr, "Could not allocate memory for from->links\n");
			fflush(stderr);
			return 0;
		}
	}
	else {
		group_t **links_tmp = realloc(from->links, sizeof(group_t *)*(size_t)(from->links_n+1));
		if (!links_tmp) {
			fprintf(stderr, "Could not reallocate memory for from->links\n");
			fflush(stderr);
			return 0;
		}
		from->links = links_tmp;
	}
	from->links[from->links_n++] = to;
	return 1;
}

void check_initial_roll(group_t *group) {
	int i;
	for (i = 0; i < group->links_n && !group->links[i]->merged; i++);
	if (i < group->links_n) {
		roll_group(group, 0);
	}
}

void set_group(group_t *group, color_t *color, int rolled, int merged) {
	group->color = color;
	group->rolled = rolled;
	group->merged = merged;
	group->size = 1;
	group->rank = 0;
	group->root = group;
	group->links_n = 0;
}

group_t *find_root(group_t *group) {
	if (group->root != group) {
		group->root = find_root(group->root);
	}
	return group->root;
}

void huedrops(int steps_idx) {
	int steps_low1 = 0, steps_low2 = 0, steps_stack_max = 0, i;
	for (i = 0; i < colors_n; i++) {
		if (colors[i].free_groups_n > 0) {
			steps_low1++;
			if (colors[i].rolls_n < colors[i].free_groups_n) {
				steps_low2++;
			}
			steps_stack_max += colors[i].free_groups_n;
		}
	}
	if (steps_low1 == 0) {
		int j;

		/* Solution found */
		steps_min = steps_idx;
		printf("%d ", steps_min);
		for (j = 0; j < steps_min; j++) {
			putchar(steps[j]);
		}
		puts("");
		fflush(stdout);
	}
	else {
		int steps_stack_size, steps_low3, colors_stack_size, j;
		group_t **steps_stack;

		/* Compute basic lower bound of number of steps remaining to complete the puzzle */
		if (color_target->free_groups_n == 0) {
			return;
		}
		steps_stack = malloc(sizeof(group_t *)*(size_t)steps_stack_max);
		if (!steps_stack) {
			fprintf(stderr, "Could not allocate memory for steps_stack\n");
			fflush(stderr);
			return;
		}
		steps_stack_size = 0;
		for (j = 0; j < colors_n; j++) {
			group_t *group;
			for (group = colors[j].roll->next; group != colors[j].roll; group = group->next) {
				steps_stack[steps_stack_size++] = group;
			}
		}
		for (j = 0; j < steps_stack_size; j++) {
			int k;
			for (k = 0; k < steps_stack[j]->links_n; k++) {
				if (steps_stack[j]->links[k]->rolled == tiles_n) {
					if (steps_stack[j]->rolled < steps_idx) {
						steps_stack[j]->links[k]->rolled = steps_idx+1;
					}
					else {
						steps_stack[j]->links[k]->rolled = steps_stack[j]->rolled+1;
					}
					steps_stack[steps_stack_size++] = steps_stack[j]->links[k];
				}
			}
		}
		if (steps_stack_size < steps_stack_max) {
			return;
		}
		steps_low3 = steps_stack[steps_stack_size-1]->rolled;
		if (color_target->rolls_n == 0) {
			int k;
			for (k = steps_stack_size-1; k >= 0 && steps_stack[k]->color != color_target && steps_stack[k]->rolled == steps_low3; k--);
			if (k < 0 || steps_stack[k]->rolled < steps_low3) {
				steps_low3++;
			}
		}
		if (steps_low1 > steps_low2) {
			steps_low3--;
		}
		for (j = steps_stack_size-1; j >= 0 && steps_stack[j]->rolled > steps_idx; j--) {
			steps_stack[j]->rolled = tiles_n;
		}
		free(steps_stack);

		/* Prune the current branch if best number of steps cannot be beaten */
		if (steps_low1+steps_low3 >= steps_min) {
			return;
		}

		/* Allocate memory for colors stack if necessary */
		colors_stack_size = steps_idx*colors_n;
		if (colors_stack_size == colors_stack_max) {
			if (colors_stack_max == 0) {
				colors_stack = malloc(sizeof(int)*(size_t)colors_n);
				if (!colors_stack) {
					fprintf(stderr, "Could not allocate memory for colors_stack\n");
					fflush(stderr);
					return;
				}
			}
			else {
				int *colors_stack_tmp = realloc(colors_stack, sizeof(int)*(size_t)(colors_stack_max+colors_n));
				if (!colors_stack) {
					fprintf(stderr, "Could not reallocate memory for colors_stack\n");
					fflush(stderr);
					return;
				}
				colors_stack = colors_stack_tmp;
			}
			colors_stack_max += colors_n;
		}

		/* Sort colors by rolls size descending/number of rolls ascending/target color first */
		for (j = 0; j < colors_n; j++) {
			colors_stack[j+colors_stack_size] = j;
		}
		qsort(colors_stack+colors_stack_size, (size_t)colors_n, sizeof(int), compare_colors);

		/* Merge neighbouring groups for each color in turn */
		for (j = 0; j < colors_n; j++) {
			merge_color(steps_idx, colors+colors_stack[j+colors_stack_size]);
		}
	}
}

int compare_colors(const void *a, const void *b) {
	const int *int_a = (const int *)a, *int_b = (const int *)b;
	if (colors[*int_a].rolls_size != colors[*int_b].rolls_size) {
		return colors[*int_b].rolls_size-colors[*int_a].rolls_size;
	}
	if (colors[*int_a].rolls_n != colors[*int_b].rolls_n) {
		return colors[*int_a].rolls_n-colors[*int_b].rolls_n;
	}
	if (colors+*int_a == color_target) {
		return -1;
	}
	if (colors+*int_b == color_target) {
		return 1;
	}
	return 0;
}

void merge_color(int steps_idx, color_t *color) {
	int rolls_n, rolls_size, i;
	group_t *roll_next, *roll_last, *merge_last, *group;
	if (color->rolls_n == 0) {
		return;
	}

	/* Add color to solution */
	steps[steps_idx] = color->symbol;

	/* Merge rolled groups and roll their not yet merged neighbours */
	roll_next = color->roll->next;
	roll_last = color->roll->last;
	rolls_n = color->rolls_n;
	rolls_size = color->rolls_size;
	merge_last = color->merge->last;
	color->rolls_n = 0;
	color->rolls_size = 0;
	set_chain(color->roll);
	chain_group_last(roll_next, merge_last);
	chain_group_next(roll_last, color->merge);
	for (group = roll_next; group != color->merge; group = group->next) {
		group->color->free_groups_n--;
		group->merged = 1;
	}
	for (group = roll_next; group != color->merge; group = group->next) {
		int j;
		for (j = 0; j < group->links_n; j++) {
			check_roll(group->links[j], steps_idx+1);
		}
	}

	/* Make solver advance to the next step */
	huedrops(steps_idx+1);

	/* Unroll not yet merged neighbours and free merged groups */
	for (i = 0; i < colors_n; i++) {
		check_unroll(colors+i, steps_idx+1);
	}
	for (group = roll_next; group != color->merge; group = group->next) {
		group->merged = 0;
		group->color->free_groups_n++;
	}
	chain_group_next(merge_last, color->merge);
	chain_group_next(roll_last, color->roll);
	chain_group_last(roll_next, color->roll);
	color->rolls_size = rolls_size;
	color->rolls_n = rolls_n;
}

void check_roll(group_t *group, int rolled) {
	if (group->rolled == tiles_n) {
		roll_group(group, rolled);
	}
}

void roll_group(group_t *group, int rolled) {
	group->color->rolls_n++;
	group->color->rolls_size += group->size;
	group->rolled = rolled;
	chain_group(group, group->color->roll->last, group->color->roll);
}

void check_unroll(color_t *color, int rolled) {
	group_t *group, *group_last;
	for (group = color->roll->last; group != color->roll && group->rolled == rolled; group = group_last) {
		group_last = group->last;
		unroll_group(group);
	}
}

void unroll_group(group_t *group) {
	unchain_group(group);
	group->rolled = tiles_n;
	group->color->rolls_size -= group->size;
	group->color->rolls_n--;
}

void set_chain(group_t *chain) {
	chain->last = chain;
	chain->next = chain;
}

void chain_group(group_t *group, group_t *last, group_t *next) {
	chain_group_last(group, last);
	chain_group_next(group, next);
}

void chain_group_last(group_t *group, group_t *last) {
	group->last = last;
	last->next = group;
}

void chain_group_next(group_t *group, group_t *next) {
	group->next = next;
	next->last = group;
}

void unchain_group(group_t *group) {
	group->last->next = group->next;
	group->next->last = group->last;
}

void free_groups(void) {
	int i;
	for (i = 0; i < groups_n; i++) {
		if (groups[i]->links_n > 0) {
			free(groups[i]->links);
		}
	}
}
