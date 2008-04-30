double fasttanh(double x) {
  double x2 = x*x;
  return x*(27+x2)/(27+9*x2);
}
