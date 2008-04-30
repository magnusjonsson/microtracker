
double lagrange4s(double* points, double x) {
  /* Magnus paper solution */
  double D = points[1];
  double B = (points[0]+points[2])*0.5-D;
  double tmp0 = points[0]-D-B;
  double tmp3 = points[3]-D-4*B;
  double A      = 0.166666666*tmp3+0.333333333*tmp0;
  double minusC = A+tmp0;
  return ((A*x+B)*x-minusC)*x+D;
}

void lagrange4coeffs(double* coeffs, double x) 
{
  coeffs[0]=((-1.0/6.0*x+1.0/2.0)*x-1.0/3.0)*x;
  coeffs[1]=(( 1.0/2.0*x-1.0    )*x-1.0/2.0)*x+1.0;
  coeffs[2]=((-1.0/2.0*x+1.0/2.0)*x+1.0    )*x;
  coeffs[3]=(( 1.0/6.0*x        )*x-1.0/6.0)*x;
}
