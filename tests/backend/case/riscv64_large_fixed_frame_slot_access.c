int main(void) {
  int fixed[1501];

  fixed[11] = 13;
  fixed[1373] = fixed[11] + 17;
  fixed[1500] = fixed[1373] + fixed[11];

  return fixed[1500] - 43;
}
