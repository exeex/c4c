int published_global;

int publish_selected_global(int flag) {
  int selected = flag ? 19 : 23;
  published_global = selected;
  return selected;
}
