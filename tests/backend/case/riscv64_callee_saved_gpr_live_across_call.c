int perturb_value(int value) {
  int doubled;
  int adjusted;

  doubled = value + value;
  adjusted = doubled + 5;
  return adjusted - 1;
}

int keep_value_across_call(int seed) {
  int carry;
  int call_result;

  carry = seed + 17;
  call_result = perturb_value(seed - 1);
  return carry + call_result;
}

int main(void) {
  int result;

  result = keep_value_across_call(6);
  if (result != 37)
    return 1;
  return 0;
}
