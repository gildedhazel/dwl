/* ************************************************************************** */
/*                                                 @@@            @@@@@@@@    */
/*                                                  @@@          @@@@@@@@@@   */
/*                                                   @@!         @@!   @@@@   */
/*                                                    !@!        !@!  @!@!@   */
/*   btrtile.h                                         @!!       @!@ @! !@!   */
/*                                                      !!!      !@!!!  !!!   */
/*   By: julmajustus <julmajustus@tutanota.com>          !!:     !!:!   !!!   */
/*                                                        ::!    :!:    !:!   */
/*   Created: 2024/12/15 00:26:07 by julmajustus           ::    ::::::: ::   */
/*   Updated: 2026/09/21 21:20:08 by julmajustus            : :   : : :  :    */
/*                                                                            */
/* ************************************************************************** */

typedef struct LayoutNode {
	unsigned int is_client_node;
	unsigned int is_split_vertically;
	unsigned int keep_dir;
	float split_ratio;
	struct wlr_box box;
	struct LayoutNode *left; /* left or top */
	struct LayoutNode *right; /* right or bottom */
	struct LayoutNode *split_node; /* parent */
	Client *client;
} LayoutNode;

static void apply_boxes(Monitor *m, LayoutNode *node);
static void apply_layout(Monitor *m, LayoutNode *node,
						struct wlr_box area, unsigned int is_root);
static void btrtile(Monitor *m);
static int clamp_split(int total, int mid, int lmin, int rmin, int g);
static LayoutNode *create_client_node(Client *c);
static LayoutNode *create_split_node(unsigned int is_split_vertically,
									LayoutNode *left, LayoutNode *right);
static void destroy_node(LayoutNode *node);
static void destroy_tree(Monitor *m);
static LayoutNode *find_suitable_split(Monitor *m, LayoutNode *start,
                                       unsigned int need_vertical, int focused_on_left);
static int half_gap(Monitor *m);
static int in_tree(Monitor *m, Client *c);
static void init_tree(Monitor *m);
static void insert_client(Monitor *m, Client *target, Client *new_client);
static void layout_boxes(Monitor *m, LayoutNode *node,
						struct wlr_box area, unsigned int is_root);
static void layout_node(Monitor *m, LayoutNode *node,
						struct wlr_box area, unsigned int is_root);
static void node_min_size(Monitor *m, LayoutNode *node, int *w, int *h);
static int outer_gap(Monitor *m);
static void remove_client(Monitor *m, Client *c);
static void remove_leaf(Monitor *m, Client *c);
static void resize_split(unsigned int need_vertical, double delta, int in_pixels);
static void resize_split_px(unsigned int need_vertical, double pixels);
static void setratio_h(const Arg *arg);
static void setratio_v(const Arg *arg);
static Client *split_target(Monitor *m);
static void swapclients(const Arg *arg);
static void swapsplit(const Arg *arg);
static void togglesplit(const Arg *arg);
static LayoutNode *tree_root(LayoutNode *n);
static void unlink_client(Client *c);
static unsigned int visible_count(LayoutNode *node, Monitor *m);
static LayoutNode *visible_parent(Monitor *m, LayoutNode *n);
static int wants_vertical(int width, int height);
static Client *xytoclient(double x, double y);

static double resize_last_update_x, resize_last_update_y;
static uint32_t last_resize_time = 0;
static int dir_changed;

/* give every visible client the box of its leaf */
void
apply_boxes(Monitor *m, LayoutNode *node)
{
	Client *c;

	if (!node) {
		return;
    }

	if (node->is_client_node) {
		c = node->client;
		if (c && VISIBLEON(c, m) && !c->isfloating && !c->isfullscreen) {
			resize(c, node->box, 0);
        }

		return;
	}

	apply_boxes(m, node->left);
	apply_boxes(m, node->right);
}

void
apply_layout(Monitor *m, LayoutNode *node,
             struct wlr_box area, unsigned int is_root)
{
	layout_boxes(m, node, area, is_root);
	apply_boxes(m, node);
}

void
btrtile(Monitor *m)
{
	Client *c, *target;
	int n = 0;

	if (!m) {
		return;
    }

	/* Remove floating and moved clients */
	wl_list_for_each(c, &clients, link) {
		if (c->mon != m || c->isfloating) {
			remove_client(m, c);
        }
	}

	layout_boxes(m, m->root, m->w, 1);

	/* Insert visible clients that are not part of the tree */
	target = split_target(m);
	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, m) || c->isfloating || c->isfullscreen) {
			continue;
        }

		if (c->node && !in_tree(m, c)) {
			unlink_client(c);
        }

		if (!c->node) {
			insert_client(m, target, c);
			layout_boxes(m, m->root, m->w, 1);
			target = c;
		}
		n++;
	}

	if (n == 0) {
		return;
    }

	apply_boxes(m, m->root);
}

/* Keep both sides of a split at least as large as min hint size */
int
clamp_split(int total, int mid, int lmin, int rmin, int g)
{
	int lo = lmin + g;
    int hi = total - rmin - g;

	if (lo <= hi) {
		return MAX(lo, MIN(mid, hi));
    }

	if (lmin + rmin <= 0) {
		return mid;
    }

	return (int)((double)total * lmin / (lmin + rmin));
}

LayoutNode *
create_client_node(Client *c)
{
	LayoutNode *node = calloc(1, sizeof(LayoutNode));

	if (!node) {
		return NULL;
    }

	node->is_client_node = 1;
	node->split_ratio = 0.5f;
	node->client = c;
	c->node = node;

	return node;
}

LayoutNode *
create_split_node(unsigned int is_split_vertically,
				LayoutNode *left, LayoutNode *right)
{
	LayoutNode *node = calloc(1, sizeof(LayoutNode));

	if (!node) {
		return NULL;
    }

	node->is_client_node = 0;
	node->split_ratio = 0.5f;
	node->is_split_vertically = is_split_vertically;
	node->left = left;
	node->right = right;

	if (left) {
		left->split_node = node;
    }

	if (right) {
		right->split_node = node;
    }

	return node;
}

void
destroy_node(LayoutNode *node)
{
	if (!node) {
		return;
    }

	if (node->is_client_node) {
		if (node->client) {
			node->client->node = NULL;
        }
	}
    else {
		destroy_node(node->left);
		destroy_node(node->right);
	}

	free(node);
}

void
destroy_tree(Monitor *m)
{
	if (!m || !m->root) {
		return;
    }

	destroy_node(m->root);
	m->root = NULL;
}

LayoutNode *
find_suitable_split(Monitor *m, LayoutNode *start_node,
		unsigned int need_vertical, int focused_on_left)
{
	LayoutNode *n = start_node, *child = NULL;

	if (!m) {
		return NULL;
    }

	if (n && n->is_client_node) {
		child = n;
		n = n->split_node;
	}

	while (n) {
		if (!n->is_client_node && n->is_split_vertically == need_vertical
				&& visible_count(n->left, m) > 0
				&& visible_count(n->right, m) > 0) {
			if ((focused_on_left && n->left == child) ||
				(!focused_on_left && n->right == child))
				return n;
		}
		child = n;
		n = n->split_node;
	}

	return NULL;
}

#ifdef BTRTILE_GAPS
int
half_gap(Monitor *m)
{
	return m->gaps ? (int)gappx / 2 : 0;
}
#else
int
half_gap(Monitor *m)
{
	return 0;
}
#endif

int
in_tree(Monitor *m, Client *c)
{
	return m && m->root && c && c->node && tree_root(c->node) == m->root;
}

void
init_tree(Monitor *m)
{
	if (m) {
		m->root = NULL;
    }
}


void
insert_client(Monitor *m, Client *target, Client *new_client)
{
	LayoutNode *leaf, *tnode, *split, *parent;
	struct wlr_box b;
	int vertical, new_first;

	if (new_client->node) {
		unlink_client(new_client);
    }

	if (!(leaf = create_client_node(new_client))) {
		return;
    }

	/* If no root, new client becomes the root. */
	if (!m->root) {
		m->root = leaf;
		return;
	}

	if (!in_tree(m, target)) {
		vertical = wants_vertical(m->w.width, m->w.height);
		if (!(split = create_split_node(vertical, m->root, leaf))) {
			goto fail;
        }

		m->root = split;
		return;
	}

	tnode = target->node;
	b = tnode->box;
	vertical = wants_vertical(b.width, b.height);

	/* Pick the side of the new client */
	if (force_split == 1) {
		new_first = 1;
    }
	else if (force_split == 2) {
		new_first = 0;
    }
	else if (vertical) {
		new_first = cursor->x <= b.x + b.width / 2.0;
    }
	else {
		new_first = cursor->y <= b.y + b.height / 2.0;
    }

	parent = tnode->split_node;
	split = new_first ? create_split_node(vertical, leaf, tnode)
			: create_split_node(vertical, tnode, leaf);
	if (!split) {
		goto fail;
    }

	split->split_node = parent;
	if (!parent) {
		m->root = split;
    }
	else if (parent->left == tnode) {
		parent->left = split;
    }
	else {
		parent->right = split;
    }

	return;

fail:
	new_client->node = NULL;
	free(leaf);
}

/* compute the box of every node */
void
layout_boxes(Monitor *m, LayoutNode *node, struct wlr_box area, unsigned int is_root)
{
	int i;

	for (i = 0; i < 4; i++) {
		dir_changed = 0;
		layout_node(m, node, area, is_root);

		if (!dir_changed) {
			break;
        }
	}
}

void
layout_node(Monitor *m, LayoutNode *node, struct wlr_box area, unsigned int is_root)
{
	unsigned int left_count, right_count;
	int mid, lw, lh, rw, rh, vertical, og, g = half_gap(m);
	float ratio;
	struct wlr_box left_area, right_area;

	if (!node) {
		return;
    }

	if (is_root) {
		og = outer_gap(m);
		area.x += og;
		area.y += og;
		area.width -= 2 * og;
		area.height -= 2 * og;
	}
	node->box = area;

	if (node->is_client_node) {
		return;
    }

	/* For a split node, see how many visible clients are on each side */
	left_count = visible_count(node->left, m);
	right_count = visible_count(node->right, m);
	if (left_count == 0 && right_count == 0) {
		return;
    }

	if (right_count == 0) {
		layout_node(m, node->left, area, 0);
		return;
	}

	if (left_count == 0) {
		layout_node(m, node->right, area, 0);
		return;
	}

	/* Visible clients on both sides */
	if (!preserve_split && !node->keep_dir) {
		vertical = wants_vertical(area.width, area.height);
		if ((unsigned int)vertical != node->is_split_vertically) {
			dir_changed = 1;
        }
		node->is_split_vertically = vertical;
	}

	ratio = node->split_ratio;
	if (ratio < 0.05f) {
		ratio = 0.05f;
    }

	if (ratio > 0.95f) {
		ratio = 0.95f;
    }

	node_min_size(m, node->left, &lw, &lh);
	node_min_size(m, node->right, &rw, &rh);
	left_area = right_area = area;

	if (node->is_split_vertically) {
		mid = clamp_split(area.width, (int)(area.width * ratio), lw, rw, g);
		left_area.width = mid - g;
		right_area.x = area.x + mid + g;
		right_area.width = area.width - mid - g;
	} else {
		mid = clamp_split(area.height, (int)(area.height * ratio), lh, rh, g);
		left_area.height = mid - g;
		right_area.y = area.y + mid + g;
		right_area.height = area.height - mid - g;
	}

	layout_node(m, node->left, left_area, 0);
	layout_node(m, node->right, right_area, 0);
}

/* Smallest size a subtree can be given without violating size hints */
void
node_min_size(Monitor *m, LayoutNode *node, int *w, int *h)
{
	Client *c;
	struct wlr_box min = {0}, max = {0};
	int lw, lh, rw, rh, g;

	*w = *h = 0;
	if (!node) {
		return;
    }

	if (node->is_client_node) {
		c = node->client;

		if (!c || !VISIBLEON(c, m) || c->isfloating || c->isfullscreen) {
			return;
        }

		client_get_size_hints(c, &max, &min);
		*w = MAX(min.width, 1) + 2 * (int)c->bw;
		*h = MAX(min.height, 1) + 2 * (int)c->bw;

		return;
	}

	node_min_size(m, node->left, &lw, &lh);
	node_min_size(m, node->right, &rw, &rh);
	if (!lw) {
		*w = rw;
		*h = rh;
		return;
	}
	if (!rw) {
		*w = lw;
		*h = lh;
		return;
	}

	g = 2 * half_gap(m);
	if (node->is_split_vertically) {
		*w = lw + rw + g;
		*h = MAX(lh, rh);
	}
    else {
		*w = MAX(lw, rw);
		*h = lh + rh + g;
	}
}

#ifdef BTRTILE_GAPS
int
outer_gap(Monitor *m)
{
	return m->gaps ? (int)gappx : 0;
}

#else
int
outer_gap(Monitor *m)
{
	return 0;
}
#endif

/* Remove c if in the tree */
void
remove_client(Monitor *m, Client *c)
{
	if (in_tree(m, c)) {
		remove_leaf(m, c);
    }
}

void
remove_leaf(Monitor *m, Client *c)
{
	LayoutNode *n = c->node, *p, *sib, *gp;

	if (!n) {
		return;
    }

	c->node = NULL;

	if (!(p = n->split_node)) {
		m->root = NULL;
		free(n);
		return;
	}

	sib = (p->left == n) ? p->right : p->left;
	gp = p->split_node;
	sib->split_node = gp;
	if (!gp) {
		m->root = sib;
    }
	else if (gp->left == p) {
		gp->left = sib;
    }
	else {
		gp->right = sib;
    }

	free(n);
	free(p);
}

/* In pixel mode delta is a pointer movement, otherwise it is a change of the split ratio. */
void
resize_split(unsigned int need_vertical, double delta, int in_pixels)
{
	Client *sel;
	LayoutNode *split;
	double extent, ratio;

	if (!selmon || !selmon->lt[selmon->sellt]->arrange) {
		return;
    }

	sel = focustop(selmon);
	if (!in_tree(selmon, sel)) {
		return;
    }

	if (in_pixels && delta == 0.0) {
		return;
    }

	split = find_suitable_split(selmon, sel->node, need_vertical, delta >= 0.0);
	if (!split) {
		split = find_suitable_split(selmon, sel->node, need_vertical, delta < 0.0);
    }

	if (!split) {
		return;
    }

	if (in_pixels) {
		extent = need_vertical ? split->box.width : split->box.height;
		if (extent <= 0.0) {
			return;
        }

		delta /= extent;
	}

	ratio = (delta != 0.0) ? split->split_ratio + delta : 0.5;
	ratio = MAX(0.05, MIN(ratio, 0.95));
	split->split_ratio = (float)ratio;

	apply_layout(selmon, selmon->root, selmon->w, 1);
}

void
resize_split_px(unsigned int need_vertical, double pixels)
{
	resize_split(need_vertical, pixels, 1);
}

void
setratio_h(const Arg *arg)
{
	resize_split(1, arg->f, 0);
}

void
setratio_v(const Arg *arg)
{
	resize_split(0, arg->f, 0);
}

Client *
split_target(Monitor *m)
{
	Client *c;

	if (!use_active_for_splits && (c = xytoclient(cursor->x, cursor->y))
			&& in_tree(m, c)) {
		return c;
    }

	wl_list_for_each(c, &fstack, flink) {
		if (VISIBLEON(c, m) && !c->isfloating && !c->isfullscreen && in_tree(m, c)) {
			return c;
        }
	}

	return NULL;
}

void
swapclients(const Arg *arg)
{
	Client *c, *target = NULL, *sel = focustop(selmon);
	LayoutNode *sel_node, *target_node;
	int closest_dist = INT_MAX, dist, sel_center_x, sel_center_y,
	cand_center_x, cand_center_y;

	if (!sel || sel->isfullscreen || !in_tree(selmon, sel)
			|| !selmon->lt[selmon->sellt]->arrange) {
		return;
    }

	/* Get the center coordinates of the selected client */
	sel_center_x = sel->geom.x + sel->geom.width / 2;
	sel_center_y = sel->geom.y + sel->geom.height / 2;

	wl_list_for_each(c, &clients, link) {
		if (!VISIBLEON(c, selmon) || c->isfloating || c->isfullscreen || c == sel) {
			continue;
        }

		/* Get the center of candidate client */
		cand_center_x = c->geom.x + c->geom.width / 2;
		cand_center_y = c->geom.y + c->geom.height / 2;

		/* Check that the candidate lies in the requested direction. */
		switch (arg->ui) {
			case 0:
				if (cand_center_x >= sel_center_x) {
					continue;
                }
				break;
			case 1:
				if (cand_center_x <= sel_center_x) {
					continue;
                }
				break;
			case 2:
				if (cand_center_y >= sel_center_y) {
					continue;
                }
				break;
			case 3:
				if (cand_center_y <= sel_center_y) {
					continue;
                }
				break;
			default:
				continue;
		}

		/* Get distance between the centers */
		dist = abs(sel_center_x - cand_center_x) + abs(sel_center_y - cand_center_y);
		if (dist < closest_dist) {
			closest_dist = dist;
			target = c;
		}
	}

	/* If target is found, swap the two clients' leaves in the layout tree */
	if (target && in_tree(selmon, target)) {
		sel_node = sel->node;
		target_node = target->node;
		sel_node->client = target;
		target_node->client = sel;
		sel->node = target_node;
		target->node = sel_node;
		arrange(selmon);
	}
}

/* Mirror the split */
void
swapsplit(const Arg *arg)
{
	Client *sel = focustop(selmon);
	LayoutNode *p, *tmp;

	if (!in_tree(selmon, sel) || !(p = visible_parent(selmon, sel->node))) {
		return;
    }

	tmp = p->left;
	p->left = p->right;
	p->right = tmp;
	p->split_ratio = 1.0f - p->split_ratio;
	arrange(selmon);
}

/* Flip the split direction of the split node */
void
togglesplit(const Arg *arg)
{
	Client *sel = focustop(selmon);
	LayoutNode *p;

	if (!in_tree(selmon, sel) || !(p = visible_parent(selmon, sel->node))) {
		return;
    }

	p->is_split_vertically = !p->is_split_vertically;
	p->keep_dir = 1;
	arrange(selmon);
}

LayoutNode *
tree_root(LayoutNode *n)
{
	while (n && n->split_node) {
		n = n->split_node;
    }

	return n;
}

/* Remove c */
void
unlink_client(Client *c)
{
	LayoutNode *root;
	Monitor *m;

	if (!c || !c->node) {
		return;
    }

	root = tree_root(c->node);
	wl_list_for_each(m, &mons, link) {
		if (m->root == root) {
			remove_leaf(m, c);
			return;
		}
	}

	c->node->client = NULL;
	c->node = NULL;
}

unsigned int
visible_count(LayoutNode *node, Monitor *m)
{
	Client *c;

	if (!node) {
		return 0;
    }

	/* Check if this client is visible. */
	if (node->is_client_node) {
		c = node->client;
		if (c && VISIBLEON(c, m) && !c->isfloating && !c->isfullscreen) {
			return 1;
        }

		return 0;
	}

	/* Else it's a split node. */
	return visible_count(node->left, m) + visible_count(node->right, m);
}

/* get nearest ancestor of node that splits visible clients on both sides */
LayoutNode *
visible_parent(Monitor *m, LayoutNode *n)
{
	for (n = n->split_node; n; n = n->split_node) {
		if (visible_count(n->left, m) > 0 && visible_count(n->right, m) > 0) {
			return n;
        }
	}

	return NULL;
}

int
wants_vertical(int width, int height)
{
	return (double)width >= (double)height * split_width_multiplier;
}

/* get tiled client under the point, or the closest one */
Client *
xytoclient(double x, double y) {
	Monitor *m = xytomon(x, y);
	Client *c, *closest = NULL;
	double dist, mindist = INT_MAX, dx, dy;

	wl_list_for_each_reverse(c, &clients, link) {
		if (VISIBLEON(c, m) && !c->isfloating && !c->isfullscreen && c->node &&
			x >= c->geom.x && x <= (c->geom.x + c->geom.width) &&
			y >= c->geom.y && y <= (c->geom.y + c->geom.height)){
			return c;
		}
	}

	/* If no client was found at cursor position fallback to closest. */
	wl_list_for_each_reverse(c, &clients, link) {
		if (VISIBLEON(c, m) && !c->isfloating && !c->isfullscreen && c->node) {
			dx = 0, dy = 0;

			if (x < c->geom.x) {
				dx = c->geom.x - x;
            }
			else if (x > (c->geom.x + c->geom.width)) {
				dx = x - (c->geom.x + c->geom.width);
            }

			if (y < c->geom.y) {
				dy = c->geom.y - y;
            }
			else if (y > (c->geom.y + c->geom.height)) {
				dy = y - (c->geom.y + c->geom.height);
            }

			dist = dx * dx + dy * dy;
			if (dist < mindist) {
				mindist = dist;
				closest = c;
			}
		}
	}

	return closest;
}
