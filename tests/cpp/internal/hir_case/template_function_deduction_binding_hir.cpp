struct Probe {
  int value;
};

Probe make_probe() {
  Probe tmp;
  tmp.value = 7;
  return tmp;
}

template <class T>
int forward_pick(T&& probe) {
  return probe.value;
}

template <class T>
int read_ptr(T* probe) {
  return probe->value;
}

int main() {
  Probe local{5};
  return forward_pick(local) + forward_pick(make_probe()) + read_ptr(&local);
}
