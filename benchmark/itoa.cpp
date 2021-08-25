#include <cstdint>

static uint16_t MAT[] = {0x3030,
    0x3130,
    0x3230,
    0x3330,
    0x3430,
    0x3530,
    0x3630,
    0x3730,
    0x3830,
    0x3930,
    0x3031,
    0x3131,
    0x3231,
    0x3331,
    0x3431,
    0x3531,
    0x3631,
    0x3731,
    0x3831,
    0x3931,
    0x3032,
    0x3132,
    0x3232,
    0x3332,
    0x3432,
    0x3532,
    0x3632,
    0x3732,
    0x3832,
    0x3932,
    0x3033,
    0x3133,
    0x3233,
    0x3333,
    0x3433,
    0x3533,
    0x3633,
    0x3733,
    0x3833,
    0x3933,
    0x3034,
    0x3134,
    0x3234,
    0x3334,
    0x3434,
    0x3534,
    0x3634,
    0x3734,
    0x3834,
    0x3934,
    0x3035,
    0x3135,
    0x3235,
    0x3335,
    0x3435,
    0x3535,
    0x3635,
    0x3735,
    0x3835,
    0x3935,
    0x3036,
    0x3136,
    0x3236,
    0x3336,
    0x3436,
    0x3536,
    0x3636,
    0x3736,
    0x3836,
    0x3936,
    0x3037,
    0x3137,
    0x3237,
    0x3337,
    0x3437,
    0x3537,
    0x3637,
    0x3737,
    0x3837,
    0x3937,
    0x3038,
    0x3138,
    0x3238,
    0x3338,
    0x3438,
    0x3538,
    0x3638,
    0x3738,
    0x3838,
    0x3938,
    0x3039,
    0x3139,
    0x3239,
    0x3339,
    0x3439,
    0x3539,
    0x3639,
    0x3739,
    0x3839,
    0x3939};
int I2A_64(uint64_t i, char* a)
{
  if (i < (uint64_t)1000000000000) {
    if (i < 100000000) {
      uint32_t _i = i;
      if (_i < 10) {
        *a = _i + '0';
        return 1;
      } else if (_i < 100) {
        *(uint16_t*)a = MAT[_i];
        return 2;
      } else if (_i < 10000) {
        uint16_t A = _i / 100, B = _i - A * 100;
        if (i < 1000) {
          *a = A + '0';
          *(uint16_t*)&a[1] = MAT[B];
          return 3;
        } else {
          *(uint16_t*)&a[2] = MAT[B];
          *(uint16_t*)a = MAT[A];
          return 4;
        }
      } else {
        uint16_t A = _i / 10000, B = _i - A * 10000, C = B / 100, D = B - C * 100;
        if (A >= 100) {
          /*12345678*/
          if (A >= 1000) {
            *(uint16_t*)a = MAT[A / 100];
            *(uint16_t*)&a[2] = MAT[A % 100];
            *(uint16_t*)&a[4] = MAT[C];
            *(uint16_t*)&a[6] = MAT[D];
            return 8;
          } else {
            *a = A / 100 + '0';
            *(uint16_t*)&a[1] = MAT[A % 100];
            *(uint16_t*)&a[3] = MAT[C];
            *(uint16_t*)&a[5] = MAT[D];
            return 7;
          }
        } else {
          if (A >= 10) {
            *(uint16_t*)a = MAT[A];
            *(uint16_t*)&a[2] = MAT[C];
            *(uint16_t*)&a[4] = MAT[D];
            return 6;
          } else {
            *a = A + '0';
            *(uint16_t*)&a[1] = MAT[C];
            *(uint16_t*)&a[3] = MAT[D];
            return 5;
          }
        }
      }
    } else {
      uint8_t len;
      uint32_t A = i / 100000000;
      if (A < 10) {
        *a = A + '0';
        len = 1;
      } else if (A < 100) {
        *(uint16_t*)a = MAT[A];
        len = 2;
      } else if (A < 1000) {
        uint16_t B = ((uint16_t)A) / 100;
        *a = B + '0';
        *(uint16_t*)(a + 1) = MAT[A - B * 100];
        len = 3;
      } else {
        uint16_t B = ((uint16_t)A) / 100;
        *(uint16_t*)a = MAT[B];
        *(uint16_t*)(a + 2) = MAT[A - B * 100];
        len = 4;
      }
      uint32_t _A = i - A * 100000000;
      char* p = a + len;
      uint16_t C;
      uint16_t H = _A / 10000, L = _A - 10000 * H;
      C = H / 100;
      *(uint16_t*)p = MAT[C];
      *(uint16_t*)(p + 2) = MAT[H - 100 * C];
      C = L / 100;
      *(uint16_t*)(p + 4) = MAT[C];
      *(uint16_t*)(p + 6) = MAT[L - 100 * C];
      return len + 8;
    }
  }
  // 64bit max is 1844 6744 0737 0955 1615
  else {
    uint32_t H = i / (uint64_t)1000000000000;
    unsigned char l = 0;
    if (H < 100) {
      if (H < 10) {
        *a = H + '0';
        l = 1;
      } else {
        *(uint16_t*)a = MAT[H];
        l = 2;
      }
    } else if (H < 10000) {
      if (H < 1000) {
        *a = H / 100 + '0';
        *(uint16_t*)(a + 1) = MAT[H % 100];
        l = 3;
      } else {
        *(uint16_t*)(a) = MAT[H / 100];
        *(uint16_t*)(a + 2) = MAT[H % 100];
        l = 4;
      }
    } else if (H < 1000000) {
      if (H < 100000) {
        uint16_t v = H / 10000;
        *a = v + '0';
        v = H - 10000 * v;
        *(uint16_t*)(a + 1) = MAT[v / 100];
        *(uint16_t*)(a + 3) = MAT[v % 100];
        l = 5;
      } else {
        uint16_t v = H / 10000;
        *(uint16_t*)(a) = MAT[v];
        v = H - 10000 * v;
        *(uint16_t*)(a + 2) = MAT[v / 100];
        *(uint16_t*)(a + 4) = MAT[v % 100];
        l = 6;
      }
    } else {
      if (H < 10000000) {
        uint16_t v = H / 10000;
        *a = v / 100 + '0';
        *(uint16_t*)(a + 1) = MAT[v % 100];
        v = H - 10000 * v;
        *(uint16_t*)(a + 3) = MAT[v / 100];
        *(uint16_t*)(a + 5) = MAT[v % 100];
        l = 7;
      } else {
        uint16_t v = H / 10000;
        *(uint16_t*)(a) = MAT[v / 100];
        *(uint16_t*)(a + 2) = MAT[v % 100];
        v = H - 10000 * v;
        *(uint16_t*)(a + 4) = MAT[v / 100];
        *(uint16_t*)(a + 6) = MAT[v % 100];
        l = 8;
      }
    }
    uint64_t L = i - (uint64_t)1000000000000 * H;
    /*      if(L==0)
     {
     memcpy(a+l,0,12);
     return l+12;
     }*/
    uint16_t v = L / 100000000;
    char* p = a + l;
    uint16_t v1 = v / 100;
    ((uint16_t*)(p))[0] = MAT[v1];
    ((uint16_t*)p)[1] = MAT[v - 100 * v1];
    uint32_t L1 = L - ((uint64_t)100000000) * v;
    v = L1 / 10000;
    v1 = v / 100;
    ((uint16_t*)p)[2] = MAT[v1];
    ((uint16_t*)p)[3] = MAT[v - 100 * v1];
    v = L1 - v * 10000;
    v1 = v / 100;
    ((uint16_t*)p)[4] = MAT[v1];
    ((uint16_t*)p)[5] = MAT[v - 100 * v1];
    return l + 12;
  }
}
int I2A_32(uint32_t i, char* a)
{
  if (i < 100000000) {
    uint32_t _i = i;
    if (_i < 10) {
      *a = _i + '0';
      return 1;
    } else if (_i < 100) {
      *(uint16_t*)a = MAT[_i];
      return 2;
    } else if (_i < 10000) {
      uint16_t A = _i / 100, B = _i - A * 100;
      if (i < 1000) {
        *a = A + '0';
        *(uint16_t*)&a[1] = MAT[B];
        return 3;
      } else {
        *(uint16_t*)&a[2] = MAT[B];
        *(uint16_t*)a = MAT[A];
        return 4;
      }
    } else {
      uint16_t A = _i / 10000, B = _i - A * 10000, C = B / 100, D = B - C * 100;
      if (A >= 100) {
        /*12345678*/
        if (A >= 1000) {
          *(uint16_t*)a = MAT[A / 100];
          *(uint16_t*)&a[2] = MAT[A % 100];
          *(uint16_t*)&a[4] = MAT[C];
          *(uint16_t*)&a[6] = MAT[D];
          return 8;
        } else {
          *a = A / 100 + '0';
          *(uint16_t*)&a[1] = MAT[A % 100];
          *(uint16_t*)&a[3] = MAT[C];
          *(uint16_t*)&a[5] = MAT[D];
          return 7;
        }
      } else {
        if (A >= 10) {
          *(uint16_t*)a = MAT[A];
          *(uint16_t*)&a[2] = MAT[C];
          *(uint16_t*)&a[4] = MAT[D];
          return 6;
        } else {
          *a = A + '0';
          *(uint16_t*)&a[1] = MAT[C];
          *(uint16_t*)&a[3] = MAT[D];
          return 5;
        }
      }
    }
  } else {
    uint8_t len;
    uint32_t A = i / 100000000;
    if (A < 10) {
      *a = A + '0';
      len = 1;
    } else {
      *(uint16_t*)a = MAT[A];
      len = 2;
    }
    uint32_t _A = i - A * 100000000;
    char* p = a + len;
    uint16_t C;
    uint16_t H = _A / 10000, L = _A - 10000 * H;
    C = H / 100;
    *(uint16_t*)p = MAT[C];
    *(uint16_t*)(p + 2) = MAT[H - 100 * C];
    C = L / 100;
    *(uint16_t*)(p + 4) = MAT[C];
    *(uint16_t*)(p + 6) = MAT[L - 100 * C];
    return len + 8;
  }
}
int I2A_16(uint16_t i, char* a)
{
  if (i < 10) {
    *a = i + '0';
    return 1;
  } else if (i < 100) {
    *(uint16_t*)a = MAT[i];
    return 2;
  } else if (i < 10000) {
    uint16_t A = i / 100, B = i - A * 100;
    if (i < 1000) {
      *a = A + '0';
      *(uint16_t*)&a[1] = MAT[B];
      return 3;
    } else {
      *(uint16_t*)&a[2] = MAT[B];
      *(uint16_t*)a = MAT[A];
      return 4;
    }
  } else {
    uint8_t A = i / 10000;
    uint16_t B = i - 10000 * A;
    *a = A + '0';
    *(uint16_t*)&a[1] = MAT[B / 100];
    *(uint16_t*)&a[3] = MAT[B % 100];
    return 5;
  }
}
