int nondet() { int a; return a; }
int nondet_bool() { int a; return a; }
int __cost;
int foo (int A, int B, int C, int D, int E, int F, int G, int H, int I, int J, int K, int L, int M, int N, int O, int P, int Q, int R, int S, int T, int U, int V, int W, int X, int Y, int Z, int A1, int B1) {
 goto loc_f8;

 loc_f1:
 {
 __cost++;
   if (nondet_bool()) {
    int N_ = nondet();
    int L_ = nondet();
    int O_ = J;
    int M_ = L;
    int K_ = L;
    int J_ = 1 + J;
    if (J >= 0 && I >= 1 + J) {
     J = J_;
     K = K_;
     L = L_;
     M = M_;
     N = N_;
     O = O_;
     goto loc_f1;
    }
   }
   if (nondet_bool()) {
    int Y_0 = nondet();
    int B1_ = nondet();
    int Z_ = nondet();
    int Y_ = nondet();
    int Q_ = nondet();
    int M_ = nondet();
    int L_ = nondet();
    int K_ = nondet();
    int J_ = nondet();
    int I_ = nondet();
    int C_ = nondet();
    int B_ = nondet();
    int E_ = K;
    if (B1_ >= B_ && B_ >= 2 && J_ >= 0 && J_ >= Y_0 && Y_0 >= 2 && J >= 0 && J >= I) {
     B = B_;
     C = C_;
     E = E_;
     I = I_;
     J = J_;
     K = K_;
     L = L_;
     M = M_;
     Q = Q_;
     Y = Y_;
     Z = Z_;
     B1 = B1_;
     goto loc_f12;
    }
   }
  goto end;
 }
 loc_f12:
 {
 __cost++;
   if (nondet_bool()) {
    int C_ = nondet();
    int B_ = nondet();
    int H_ = A;
    int F_ = G;
    int D_ = E;
    if (B_ >= 2 && A >= 0) {
     B = B_;
     C = C_;
     D = D_;
     F = F_;
     H = H_;
     goto loc_f12;
    }
   }
   if (nondet_bool()) {
    int Y_0 = nondet();
    int Y_1 = nondet();
    int A1_ = nondet();
    int Z_ = nondet();
    int X_ = nondet();
    int W_ = nondet();
    int V_ = nondet();
    int U_ = nondet();
    int T_ = nondet();
    int B_ = nondet();
    int E_ = 0;
    int C_ = 0;
    if (C == 0 && E == 0 && B_ >= 2 && A >= 0 && Y_0 >= 2 && Y_1 >= 2) {
     B = B_;
     C = C_;
     E = E_;
     T = T_;
     U = U_;
     V = V_;
     W = W_;
     X = X_;
     Z = Z_;
     A1 = A1_;
     goto loc_f9;
    }
   }
  goto end;
 }
 loc_f8:
 {
 __cost++;
   if (nondet_bool()) {
    int R_ = nondet();
    int Q_ = nondet();
    int P_ = nondet();
    int M_ = nondet();
    int L_ = nondet();
    int K_ = nondet();
    int I_ = nondet();
    int B_ = nondet();
    int S_ = 2;
    int J_ = 2;
    if (I_ >= 2) {
     B = B_;
     I = I_;
     J = J_;
     K = K_;
     L = L_;
     M = M_;
     P = P_;
     Q = Q_;
     R = R_;
     S = S_;
     goto loc_f1;
    }
   }
   if (nondet_bool()) {
    int Y_0 = nondet();
    int Y_1 = nondet();
    int Y_2 = nondet();
    int A1_ = nondet();
    int Z_ = nondet();
    int Y_ = nondet();
    int X_ = nondet();
    int W_ = nondet();
    int V_ = nondet();
    int U_ = nondet();
    int T_ = nondet();
    int Q_ = nondet();
    int P_ = nondet();
    int M_ = nondet();
    int L_ = nondet();
    int K_ = nondet();
    int J_ = nondet();
    int I_ = nondet();
    int B_ = nondet();
    int E_ = 0;
    int C_ = 0;
    if (0 >= Y_0 && 0 >= B_ && 0 >= Y_1 && 0 >= Y_2) {
     B = B_;
     C = C_;
     E = E_;
     I = I_;
     J = J_;
     K = K_;
     L = L_;
     M = M_;
     P = P_;
     Q = Q_;
     T = T_;
     U = U_;
     V = V_;
     W = W_;
     X = X_;
     Y = Y_;
     Z = Z_;
     A1 = A1_;
     goto loc_f9;
    }
   }
   if (nondet_bool()) {
    int A1_ = nondet();
    int Z_ = nondet();
    int Y_ = nondet();
    int X_ = nondet();
    int W_ = nondet();
    int V_ = nondet();
    int U_ = nondet();
    int T_ = nondet();
    int Q_ = nondet();
    int P_ = nondet();
    int M_ = nondet();
    int L_ = nondet();
    int K_ = nondet();
    int J_ = nondet();
    int I_ = nondet();
    int C_ = nondet();
    int E_ = 0;
    int B_ = 1;
    if (L == 0) {
     B = B_;
     C = C_;
     E = E_;
     I = I_;
     J = J_;
     K = K_;
     L = L_;
     M = M_;
     P = P_;
     Q = Q_;
     T = T_;
     U = U_;
     V = V_;
     W = W_;
     X = X_;
     Y = Y_;
     Z = Z_;
     A1 = A1_;
     goto loc_f9;
    }
   }
  goto end;
 }
 loc_f9:
 end: __VERIFIER_print_hull(__cost);
 return 0;
}
void main() {
  foo(nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet(), nondet());
}
