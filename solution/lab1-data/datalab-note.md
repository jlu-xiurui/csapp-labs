# Data Lab  
---
本实验需要解决一系列的位运算函数，并通过给定的测试用例，考察了实验者对于整形数据和浮点数据在计算机中存储形式和位运算特点的知识。
## BitXor
```  
* bitXor - x^y using only ~ and & 
*   Example: bitXor(4, 5) = 1
*   Legal ops: ~ &
*   Max ops: 14
*   Rating: 1
*/
int bitXor(int x, int y) {
    return ~(~x&~y)&(~(x&y));
}
```

该函数需要用and运算构造异或运算，考察了摩尔定律。
## tmin
```
* tmin - return minimum two's complement integer 
*   Legal ops: ! ~ & ^ | + << >>
*   Max ops: 4
*   Rating: 1
*/
int tmin(void) {
    return 0x80000000;
}
```
该函数需要返回int型数据中的最小值，即 `0x80000000`。
## isTmax
```
* isTmax - returns 1 if x is the maximum, two's complement number,
*     and 0 otherwise 
*   Legal ops: ! ~ & ^ | +
*   Max ops: 10
*   Rating: 1
*/
int isTmax(int x) {
    int mask = 0x80000000;
    int syn = !!(mask & x);
    x = x | mask;
    x = ~x+syn;
    return  !x;
}
```
该函数判断参数是否为int型数据的最大值，即 `0x7fffffff` 。可以对符号位和其他位分别进行判断，首先令syn等于x的符号位，并将x的符号位置为1。当x等于 `0x7fffffff` 和 `0xffffffff`时,对符号位至1的x取反会得到0值。因此，仅需进而判断原来的x的符号位是否为1即可。
## allOddBits
```
* allOddBits - return 1 if all odd-numbered bits in word set to 1
*   where bits are numbered from 0 (least significant) to 31 (most significant)
*   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
*   Legal ops: ! ~ & ^ | + << >>
*   Max ops: 12
*   Rating: 2
*/
int allOddBits(int x) {
    int mask = 1<<1;
    mask = mask | (mask<<2);
    mask = mask | (mask<<4);
    mask = mask | (mask<<8);
    mask = mask | (mask<<16);
    return !((mask&x)^mask);
}
```
本函数需要判断x是否为奇数为全为1的数，因此仅需将x和 `0xAAAAAAAA` 取异或即可。
## negate
```
/* 
* negate - return -x 
*   Example: negate(1) = -1.
*   Legal ops: ! ~ & ^ | + << >>
*   Max ops: 5
*   Rating: 2
*/
int negate(int x) {
    return ~x+1;
}
```
取相反数操作，根据补码规则即可轻松写出。
## isAsciiDigit
```
* isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
*   Example: isAsciiDigit(0x35) = 1.
*            isAsciiDigit(0x3a) = 0.
*            isAsciiDigit(0x05) = 0.
*   Legal ops: ! ~ & ^ | + << >>
*   Max ops: 15
*   Rating: 3
*/
int isAsciiDigit(int x) {
    int mask1 = ~0xFF;
    return !((mask1&x)+((0xF0&x)^0x30)+!((12&x)^12)+!((10&x)^10));
}
```
本函数需要判断x是否为大于等于0x30且小于等于0x39的数，在这里需要观察满足该条件的数的性质。
  
* 高24位均为0；
* 低5至8位为0011; 
* 低4位和低3位不同时为0；
* 低4位和低2位不同时为0；
  
将上述几个条件进行取或操作即可进行判断。
## conditional
```
* conditional - same as x ? y : z 
*   Example: conditional(2,4,5) = 4
*   Legal ops: ! ~ & ^ | + << >>
*   Max ops: 16
*   Rating: 3
*/
int conditional(int x, int y, int z) {
    int mask1 = !x+(~0);
    int mask2 = ~mask1;
    return y&mask1|z&mask2;
}
```
该函数需要我们实现三元运算符。当x为0时，`!x+0xffffffff`为全1掩码，当x为非0时，其为全0掩码，据此即可构造出本问题的答案。
## isLessOrEqual
```
* isLessOrEqual - if x <= y  then return 1, else return 0 
*   Example: isLessOrEqual(4,5) = 1.
*   Legal ops: ! ~ & ^ | + << >>
*   Max ops: 24
*   Rating: 3
*/
int isLessOrEqual(int x, int y) {
    int negy = (~y)+1;
    int syn = 1<<31;
    int flag1 = !!((x+negy)&syn);
    int flag2 = !(x^y);
    int flag3 = (x^y)&syn;
    return ( (!flag3)&(flag1|flag2) | !!((flag3)&(x&syn)) );
}
```
该问题的主要困难在于对于减法溢出的判断，进行分情况讨论即可避开溢出问题：  

* 当x与y同号时，不需要考虑溢出问题，当**x与y相等**或者**x减去y小于0**时x <= y成立。
* 当x与y异号时，仅当x为负数时x <= y成立。
## logicalNeg
```
* logicalNeg - implement the ! operator, using all of 
*              the legal operators except !
*   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
*   Legal ops: ~ & ^ | + << >>
*   Max ops: 12
*   Rating: 4 
*/
int logicalNeg(int x) {
  	return ((x|(~x+1))>>31)+1;
}
```
0为所有数中唯一的本身与相反数符号位均为0的数，根据此性质可以将 x 和 -x 的符号位取或，并对符号位进行符号右移扩充，使得0的运算结果仍为0，其余数的运算结果为 `0xffffffff`，对运算结果加一即可构造出逻辑非的形式。
##howManyBits
```
/* howManyBits - return the minimum number of bits required to represent x in
*             two's complement
*  Examples: howManyBits(12) = 5
*            howManyBits(298) = 10
*            howManyBits(-5) = 4
*            howManyBits(0)  = 1
*            howManyBits(-1) = 1
*            howManyBits(0x80000000) = 32
*  Legal ops: ! ~ & ^ | + << >>
*  Max ops: 90
*  Rating: 4
*/
int howManyBits(int x) {
    int flag = x>>31;
    x = (flag&~x)|(~flag&x);
    int b16 = (!!(x>>16))<<4;
    x = x >> b16;
    int b8 = (!!(x>>8))<<3;
    x = x >> b8;
    int b4 = (!!(x>>4))<<2;
    x = x >> b4;
    int b2 = (!!(x>>2))<<1;
    x = x >> b2;
    int b1 = !!(x>>1);
    x = x >> b1;
    return 1+b1+b2+b4+b8+b16+x;
}
```
这道题我一开始并没有明白题目的意思，搜索得到的答案是 **“对于负数，最少位数的二进制表示为一位符号位加上负数取反后的最高位1的位数；对于正数，最少位数的二进制表示为一位符号位加上其最高位1的位数”** 。在这里我的理解是，对于int型数据，符号位为0的最小数为0，符号位为1的最大数为-1，当非负数 x 和负数 y 满足 `x == -y-1` 时，x 和 ~y的二进制表示相同。因此，对于负数以-1为起点编码、对于正数以0为起点编码的编码方式所得到的**int类型整体2进制位数**最小。其中，对最高位1的位数的求取可以采用手动二分法得到。
## floatScale2
```
/* 
* floatScale2 - Return bit-level equivalent of expression 2*f for
*   floating point argument f.
*   Both the argument and result are passed as unsigned int's, but
*   they are to be interpreted as the bit-level representation of
*   single-precision floating point values.
*   When argument is NaN, return argument
*   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
*   Max ops: 30
*   Rating: 4
*/
unsigned floatScale2(unsigned uf) {
    int mask_exp = 0x7f800000;
    int syn = uf & 0x80000000;
    if(!((mask_exp&uf) ^ mask_exp)){
        return uf;
    }
    else if(!(mask_exp & uf)){
        uf = uf<<1;
        uf = uf | syn;
        return uf;
    }
    else{
        int val = 1<<23;
        int cur = 23;
        while(cur<31 && (val & uf)){
            uf = uf & (~val);
            val = val << 1;
            cur++;
        }
        if(cur < 31) uf |= val;
    }
    return uf;
}
```
浮点数的表现形式按照阶码格式不同分为三种，在这里利用阶码掩码对 uf 的阶码进行提取并分类讨论：  

* 对于NAN和正负无穷，阶码为全1，乘以二的结果与原二进制表示相同；
* 对于非规格化数，阶码为全0，其尾数代表了小于1的二进制小数，对其进行保持符号位的左移一位即可；
* 对于规格化数，乘以二对于尾数没有改变，仅对于阶码进行加一操作即可。
## floatFloat2Int
```
* floatFloat2Int - Return bit-level equivalent of expression (int) f
*   for floating point argument f.
*   Argument is passed as unsigned int, but
*   it is to be interpreted as the bit-level representation of a
*   single-precision floating point value.
*   Anything out of range (including NaN and infinity) should return
*   0x80000000u.
*   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
*   Max ops: 30
*   Rating: 4
*/
int floatFloat2Int(unsigned uf) {
    int mask_exp = 0x7f800000;
    int syn = uf & 0x80000000;
    uf = uf & 0x7fffffff;
    int e = (uf & mask_exp)>>23;
    int E = e - 127;
    if(E < 0) return 0;
    else if(E > 31) return 0x80000000u;
    uf = uf & (~mask_exp);
    uf |= 1<<23;
    if(E < 23) uf >>= 23 - E;
    else uf <<= E - 23;
    if(syn){
        uf = ~uf+1;
        uf |= syn;
    }
    return uf;
}
```
与上一道题相同，在这里同样按照阶码的性质进行分类讨论：  

* 当阶码的值小于0 + 127时，其值小于1，因此无法用整数进行表示，返回0即可；
* 当阶码的值大于31 + 127时，uf表达的浮点数的数值大于int可以容纳的最大数值，返回 `0x80000000u` 即可；
* 对于其余的数，其均为规格化数，因此在低23位处或1以填充隐含的1开头。当屏蔽符号位和阶码位的情况下，此时的 uf 若将其视为int类型数据，其值就等于 `(2^23)*(1+f)` 。因此，按照阶码E的实际值对 uf 进行左移或右移操作即可构造出与答案绝对值相同的int型二进制表达，接下来按照之前保存的符号位对负数进行取负操作即可得到答案。
## floatPower2
```
* floatPower2 - Return bit-level equivalent of the expression 2.0^x
*   (2.0 raised to the power x) for any 32-bit integer x.
*
*   The unsigned value that is returned should have the identical bit
*   representation as the single-precision floating-point number 2.0^x.
*   If the result is too small to be represented as a denorm, return
*   0. If too large, return +INF.
* 
*   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
*   Max ops: 30 
*   Rating: 4
*/
unsigned floatPower2(int x) {
    if(x > 0){
        if(x > 127) return 0x7f800000;
    }
    else{
        if(x < -126) return 0;
    }
    x = x + 127;
    return x<<23;
}
```
该问题比较简单，对于以2.0为底数的任何次幂，其尾数均为全零以表达1.0的整数倍。因此，首先对于溢出情况进行判断，float型数据阶码的表达范围为2^(-126)至2^(127)，对于超出该范围的 x 返回相应的溢出值即可。对于非溢出值，仅需构造其阶码即可。
