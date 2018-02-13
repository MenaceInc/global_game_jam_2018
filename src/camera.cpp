struct Camera {
    r32 x, y, target_x, target_y;
};

Camera init_camera(r32 x, r32 y) {
    Camera c = { x, y, x, y };
    return c;
}

void update_camera(Camera *c) {
    c->x += (c->target_x - c->x) * 0.12;
    c->y += (c->target_y - c->y) * 0.12;
}
