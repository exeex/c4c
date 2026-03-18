// Test: multi-hop forwarding of NTTP values through function templates.
// Similar to existing chain tests, but focused on pure NTTP forwarding depth.

template <int N>
int leaf() {
  return N;
}

template <int N>
int hop1() {
  return leaf<N>();
}

template <int N>
int hop2() {
  return hop1<N>();
}

template <int N>
int hop3() {
  return hop2<N>();
}

template <int A, int B>
int pair_sum() {
  return hop3<A>() + hop3<B>();
}

int main() {
  if (leaf<42>() != 42) return 1;
  if (hop1<42>() != 42) return 1;
  if (hop2<42>() != 42) return 2;
  if (hop3<42>() != 42) return 3;

  if (leaf<-7>() != -7) return 4;
  if (hop1<-7>() != -7) return 5;
  if (hop2<-7>() != -7) return 6;
  if (hop3<-7>() != -7) return 7;

  if (leaf<20>() != 20) return 8;
  if (hop1<20>() != 20) return 9;
  if (hop2<20>() != 20) return 10;
  if (hop3<20>() != 20) return 11;

  if (leaf<22>() != 22) return 12;
  if (hop1<22>() != 22) return 13;
  if (hop2<22>() != 22) return 14;
  if (hop3<22>() != 22) return 15;

  if (leaf<100>() != 100) return 16;
  if (hop1<100>() != 100) return 17;
  if (hop2<100>() != 100) return 18;
  if (hop3<100>() != 100) return 19;

  if (leaf<-58>() != -58) return 20;
  if (hop1<-58>() != -58) return 21;
  if (hop2<-58>() != -58) return 22;
  if (hop3<-58>() != -58) return 23;

  if (pair_sum<20, 22>() != 42) return 24;
  if (pair_sum<100, -58>() != 42) return 25;

  return 0;
}
