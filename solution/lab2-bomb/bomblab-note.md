Bomb Lab
=====

 本实验由六个子部分组成，在每个子部分中需要输入对应格式的字符串从对应的 `phase` 函数中安全返回。具体的做法就是利用 `gdb` 来观察各 `phase` 的具体细节，从而确定应当输入字符串的格式（一串特定的文字或是以 `\0` 隔开的若干整数）。 

[http://beej.us/guide/bggdb/](http://beej.us/guide/bggdb/) 是对`gdb` 命令的简要说明书，在这里列出该网站中没有介绍，但在实验中可能用到的一些命令：

* `x /<n/f/u> <addr>`：该命令可以查看地址addr中的值，其中n为所要显示的值的个数、f为显示值的格式、u为一个值所占的字节数。值得注意的是 `   $<rsg>` 可以代表寄存器rsg中存放的地址;
* `i r <rsg>`： 该命令将显示寄存器rsg中存储的值，可以用`i r a`来查看全部寄存器中的值;
* `\n`: 重复上一次输入的命令;
* `<ctrl+l>`：当运行一些命令之后gdb窗口会花屏，调用该命令即可进行清屏操作。
* `set args <lists>`：设置主函数的参数，可以创建txt文件保存已完成的部分，并将其作为函数参数。
## phase_1  
```
0x400ee0 <phase_1>      sub    $0x8,%rsp                                
0x400ee4 <phase_1+4>    mov    $0x402400,%esi                            
0x400ee9 <phase_1+9>    callq  0x401338 <strings_not_equal>              
0x400eee <phase_1+14>   test   %eax,%eax                                 
0x400ef0 <phase_1+16>   je     0x400ef7 <phase_1+23>                     
0x400ef2 <phase_1+18>   callq  0x40143a <explode_bomb>                   
0x400ef7 <phase_1+23>   add    $0x8,%rsp                                 
0x400efb <phase_1+27>   retq   
```
第一个炸弹函数比较简单，其功能是将地址0x402400处存放的字符串与输入字符串进行比较，如果不相同就会触发 `explode_bomb` 炸弹函数。通过 `x /1s 0x402400` 可以得到所应输入的字符串，如下所示：  
```
0x402400:   "Border relations with Canada have never been better."
```
## phase_2
相比第一个函数，第二个函数的汇编代码长度增加了一些，在这里将其拆分为几个部分进行介绍，在下文也将继承这种格式：
### Part1:  

```
0x400efe <phase_2+2>    sub    $0x28,%rsp
0x400f02 <phase_2+6>    mov    %rsp,%rsi 
0x400f05 <phase_2+9>    callq  0x40145c <read_six_numbers>  
```
该部分主要的工作就是调用 `<read_six_numbers>` 在栈帧中输入数据，其中`<read_six_number>`的代码如下：  

```
0x40145c <read_six_numbers>     sub    $0x18,%rsp
0x401460 <read_six_numbers+4>   mov    %rsi,%rdx                                                                        
0x401463 <read_six_numbers+7>   lea    0x4(%rsi),%rcx                                                                                 
0x401467 <read_six_numbers+11>  lea    0x14(%rsi),%rax                                                                                                                                      
0x40146b <read_six_numbers+15>  mov    %rax,0x8(%rsp)                                                                                                                                                
0x401470 <read_six_numbers+20>  lea    0x10(%rsi),%rax                                                                                                                      
0x401474 <read_six_numbers+24>  mov    %rax,(%rsp)                                                                                                                                                   
0x401478 <read_six_numbers+28>  lea    0xc(%rsi),%r9                                                                                                                                                 
0x40147c <read_six_numbers+32>  lea    0x8(%rsi),%r8                                                                                                                                                 
0x401480 <read_six_numbers+36>  mov    $0x4025c3,%esi                                                                                                                                              
0x401485 <read_six_numbers+41>  mov    $0x0,%eax                                                                                                                                                     
0x40148a <read_six_numbers+46>  callq  0x400bf0 <__isoc99_sscanf@plt>                                                                                                                                
0x40148f <read_six_numbers+51>  cmp    $0x5,%eax                                                                                                                                                     
0x401492 <read_six_numbers+54>  jg     0x401499 <read_six_numbers+61>                                                                                                                                
0x401494 <read_six_numbers+56>  callq  0x40143a <explode_bomb>
```  

函数的功能为将以主函数中输入的字符串以 `\0` 为分隔符分为6个4字节长度整形值，并存放在调用该函数时的 `%rsp` 寄存器存放的地址处。当以`"1 2 3 4 5 6"`作为主函数中的输入时，在从该函数返回phase_1后，输入 `x /6dw $rsp` 查看调用该函数后栈帧中的数据，可以得到以下输出：  

```
0x7fffffffdf70: 1       2       3       4
0x7fffffffdf80: 5       6
```
### part2:  
```
0x400f0e <phase_2+18>   je     0x400f30 <phase_2+52>
0x400f10 <phase_2+20>   callq  0x40143a <explode_bomb>
0x400f15 <phase_2+25>   jmp    0x400f30 <phase_2+52>                                                                                                                                                 
0x400f17 <phase_2+27>   mov    -0x4(%rbx),%eax                                                                                                                                                        
0x400f1a <phase_2+30>   add    %eax,%eax                                                                                                                                                             
0x400f1c <phase_2+32>   cmp    %eax,(%rbx)                                                                                                                                                            
0x400f1e <phase_2+34>   je     0x400f25 <phase_2+41>                                                                                                                                                  
0x400f20 <phase_2+36>   callq  0x40143a <explode_bomb>                                                                                                                                                
0x400f25 <phase_2+41>   add    $0x4,%rbx                                                                                                                                                              
0x400f29 <phase_2+45>   cmp    %rbp,%rbx                                                                                                                                                              
0x400f2c <phase_2+48>   jne    0x400f17 <phase_2+27>                                                                                                                                                 
0x400f2e <phase_2+50>   jmp    0x400f3c <phase_2+64>                                                                                                                                                  
0x400f30 <phase_2+52>   lea    0x4(%rsp),%rbx                                                                                                                                                         
0x400f35 <phase_2+57>   lea    0x18(%rsp),%rbp                                                                                                                                                        
0x400f3a <phase_2+62>   jmp    0x400f17 <phase_2+27> 
0x400f3c <phase_2+64>   add    $0x28,%rsp
```
该部分的功能是进行两个判断  

* 读入的第一个整数是否是1；
* 相邻的两个整数中，后一个整数的值是否为前一个整数值的2倍。  

如果其中一个判断为假则触发 `explode_bomb` ，可以将上述语句转换为类似的C语句来帮助理解（这一方法对于下面的困难问题的解决会有一定的帮助）：

```
int num[6];
read_six_numbers(input,num);
if(num[0] != 1) explode_bomb();
for(int i=0;i<5;i++){
    if(num[i+1] != num[i]*2)
        explode_bomb();
```
因此，`"1 2 4 8 16 31"` 即为本问题的答案。
## phase_3
### part1:  
```
0x400f47 <phase_3+4>    lea    0xc(%rsp),%rcx                                                                                                                                                        
0x400f4c <phase_3+9>    lea    0x8(%rsp),%rdx                                                                                                                                                         
0x400f51 <phase_3+14>   mov    $0x4025cf,%esi                                                                                                                                                         
0x400f56 <phase_3+19>   mov    $0x0,%eax                                                                                                                                                              
0x400f5b <phase_3+24>   callq  0x400bf0 <__isoc99_sscanf@plt>                                                                                                                                         
0x400f60 <phase_3+29>   cmp    $0x1,%eax                                                                                                                                                              
0x400f63 <phase_3+32>   jg     0x400f6a <phase_3+39>                                                                                                                                                  
0x400f65 <phase_3+34>   callq  0x40143a <explode_bomb>                                                                                                                                                
0x400f6a <phase_3+39>   cmpl   $0x7,0x8(%rsp)
0x400f6f <phase_3+44>   ja     0x400fad <phase_3+106>  
0x400fad <phase_3+106>  callq  0x40143a <explode_bomb> 
```
通过阅读该部分，可以看出 `phase_3` 读入了两个四字节数数存放在 `$rsp+8` 和 `$rsp+12` 处，并且如果 `$rsp+8` 处的数大于7则会引爆炸弹。
### part2:
```
0x400f71 <phase_3+46>   mov    0x8(%rsp),%eax                                                                                                                                                         
0x400f75 <phase_3+50>   jmpq   *0x402470(,%rax,8)                                                                                                                                                     
0x400f7c <phase_3+57>   mov    $0xcf,%eax                                                                                                                                                             
0x400f81 <phase_3+62>   jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400f83 <phase_3+64>   mov    $0x2c3,%eax                                                                                                                                                            
0x400f88 <phase_3+69>   jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400f8a <phase_3+71>   mov    $0x100,%eax                                                                                                                                                            
0x400f8f <phase_3+76>   jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400f91 <phase_3+78>   mov    $0x185,%eax                                                                                                                                                            
0x400f96 <phase_3+83>   jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400f98 <phase_3+85>   mov    $0xce,%eax                                                                                                                                                             
0x400f9d <phase_3+90>   jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400f9f <phase_3+92>   mov    $0x2aa,%eax                                                                                                                                                            
0x400fa4 <phase_3+97>   jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400fa6 <phase_3+99>   mov    $0x147,%eax             
0x400fab <phase_3+104>  jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400fad <phase_3+106>  callq  0x40143a <explode_bomb>                                                                                                                                                
0x400fb2 <phase_3+111>  mov    $0x0,%eax                                                                                                                                                              
0x400fb7 <phase_3+116>  jmp    0x400fbe <phase_3+123>                                                                                                                                                 
0x400fb9 <phase_3+118>  mov    $0x137,%eax                                                                                                                                                            
0x400fbe <phase_3+123>  cmp    0xc(%rsp),%eax                                                                                                                                                         
0x400fc2 <phase_3+127>  je     0x400fc9 <phase_3+134>                                                                                                                                                
0x400fc4 <phase_3+129>  callq  0x40143a <explode_bomb> 
```
很明显这是一个 `switch` 语句的基本格式，且 `$rsp+8` 中的值为输入至 `switch` 中的参数，`switch` 语句为局部变量赋值并与 `$rsp+12` 处的值比较，如相同则通过函数。通过 `x /8xg 0x402470` 查看跳转表的跳转地址：  
```
0x402470:       0x0000000000400f7c      0x0000000000400fb9
0x402480:       0x0000000000400f83      0x0000000000400f8a
0x402490:       0x0000000000400f91      0x0000000000400f98
0x4024a0:       0x0000000000400f9f      0x0000000000400fa6
```
该问题答案不唯一，任意一组跳转地址偏移量与对应的值组成的组合均可作为答案，例如：`"1 311"`。
## phase_4
```
0x401010 <phase_4+4>    lea    0xc(%rsp),%rcx                                                                                                                                                         
0x401015 <phase_4+9>    lea    0x8(%rsp),%rdx                                                                                                                                                         
0x40101a <phase_4+14>   mov    $0x4025cf,%esi                                                                                                                                                         
0x40101f <phase_4+19>   mov    $0x0,%eax                                                                                                                                                              
0x401024 <phase_4+24>   callq  0x400bf0 <__isoc99_sscanf@plt>                                                                                                                                         
0x401029 <phase_4+29>   cmp    $0x2,%eax                                                                                                                                                              
0x40102c <phase_4+32>   jne    0x401035 <phase_4+41>                                                                                                                                                  
0x40102e <phase_4+34>   cmpl   $0xe,0x8(%rsp)                                                                                                                                                         
0x401033 <phase_4+39>   jbe    0x40103a <phase_4+46>                                                                                                                                                  
0x401035 <phase_4+41>   callq  0x40143a <explode_bomb>                                                                                                                                                
0x40103a <phase_4+46>   mov    $0xe,%edx                                                                                                                                                              
0x40103f <phase_4+51>   mov    $0x0,%esi                                                                                                                                                              
0x401044 <phase_4+56>   mov    0x8(%rsp),%edi                                                                                                                                                         
0x401048 <phase_4+60>   callq  0x400fce <func4> 
0x40104d <phase_4+65>   test   %eax,%eax                                                                                                                                                              
0x40104f <phase_4+67>   jne    0x401058 <phase_4+76>                                                                                                                                                  
0x401051 <phase_4+69>   cmpl   $0x0,0xc(%rsp)                                                                                                                                                         
0x401056 <phase_4+74>   je     0x40105d <phase_4+81>                                                                                                                                                  
0x401058 <phase_4+76>   callq  0x40143a <explode_bomb> 
```  

该函数的主体部分比较简单，先是读入两个数，将其中的一个数传入 `func4` ，另一个判断其是否等于零，可以转换为下列等价C语句：  

```
void phase_4(char* input){
    int num[2];
    int len = 0;
   _iso99_sscanf(input,&len,num); 
    if(len != 2) explode_bomb();
    if(num[0] > 14) explode_bomb();
    int x = 14,y = 0,z = num[0];
    if(func4(x,y,z) != 0 || num[1] != 0) explode_bomb();
}
```  
本问题的关键之处就是在于 `func4` 函数的解读，它是一个递归函数，代码如下：  

```
0x400fd2 <func4+4>      mov    %edx,%eax                                                                                                                                                              
0x400fd4 <func4+6>      sub    %esi,%eax                                                                                                                                                              
0x400fd6 <func4+8>      mov    %eax,%ecx                                                                                                                                                              
0x400fd8 <func4+10>     shr    $0x1f,%ecx                                                                                                                                                             
0x400fdb <func4+13>     add    %ecx,%eax                                                                                                                                                              
0x400fdd <func4+15>     sar    %eax                                                                                                                                                                   
0x400fdf <func4+17>     lea    (%rax,%rsi,1),%ecx                                                                                                                                                     
0x400fe2 <func4+20>     cmp    %edi,%ecx                                                                                                                                                              
0x400fe4 <func4+22>     jle    0x400ff2 <func4+36>                                                                                                                                                    
0x400fe6 <func4+24>     lea    -0x1(%rcx),%edx                                                                                                                                                        
0x400fe9 <func4+27>     callq  0x400fce <func4>                                                                                                                                                       
0x400fee <func4+32>     add    %eax,%eax                                                                                                                                                              
0x400ff0 <func4+34>     jmp    0x401007 <func4+57>                                                                                                                                                    
0x400ff2 <func4+36>     mov    $0x0,%eax                                                                                                                                                              
0x400ff7 <func4+41>     cmp    %edi,%ecx                                                                                                                                                             
0x400ff9 <func4+43>     jge    0x401007 <func4+57>                                                                                                                                                    
0x400ffb <func4+45>     lea    0x1(%rcx),%esi                                                                                                                                                         
0x400ffe <func4+48>     callq  0x400fce <func4>                                                                                                                                                       
0x401003 <func4+53>     lea    0x1(%rax,%rax,1),%eax
```
对递归函数的汇编代码进行阅读比较困难，在这里将其转化为等价的C语句以方便理解：  

```	
int func4(int x,int y,int z){
    int ret = x - y;
    ret = ret + (ret>>31);
    ret >>= 1;
    int c = ret + y;
    if( c > z ){
	x = c - 1;
	ret = func4(x,y,z);
	ret = ret + ret;
	return ret;
    }
    ret = 0;
    return c <= z ? a : 2*a + 1;
}
```
当输入为`x = 14,y = 0,z = num[0]` 时，对该函数进行跟踪，可以发现后两个参数在递归过程中保持不变，而参数x在递归过程中的值变化情况为 `14->6->2->1`。可以发现，除了最深层的递归函数以外，`func4` 在其子函数返回时将其子函数的返回值乘2以返回其父函数，为了满足 `phase_4` 中返回值为0的条件，最深层的递归函数应当以0作为返回值。   
当 `num[0]` 等于7、3、1时，最深层的递归函数结束时会使得 `c <= z` 条件成立，以返回0值，使得 `func4` 最终以0值返回 `phase_4`，因此本问题的答案可以是 `"7 0"`、 `"3 0"`、`"1 0"`中的任意一个。
## phase_5
本问题中存在两部分比较特殊的部分，其功能是检查函数调用过程中是否发生了栈帧破坏，对于函数的主体功能影响不大：  

```
0x40106a <phase_5+8>    mov    %fs:0x28,%rax                                                                                                                                                          
0x401073 <phase_5+17>   mov    %rax,0x18(%rsp)
...
0x4010d9 <phase_5+119>  mov    0x18(%rsp),%rax
0x4010de <phase_5+124>  xor    %fs:0x28,%rax                                                                                                                                                          
0x4010e7 <phase_5+133>  je     0x4010ee <phase_5+140>                                                                                                                                                 
0x4010e9 <phase_5+135>  callq  0x400b30 <__stack_chk_fail@plt>
```
### part1:  
```
0x401067 <phase_5+5>    mov    %rdi,%rbx 
0x401078 <phase_5+22>   xor    %eax,%eax                                                                                                                                                             
0x40107a <phase_5+24>   callq  0x40131b <string_length>                                                                                                                                               
0x40107f <phase_5+29>   cmp    $0x6,%eax                                                                                                                                                              
0x401082 <phase_5+32>   je     0x4010d2 <phase_5+112>                                                                                                                                                 
0x401084 <phase_5+34>   callq  0x40143a <explode_bomb>                                                                                                                                                
0x401089 <phase_5+39>   jmp    0x4010d2 <phase_5+112>  
```
该函数的第一个部分是对其输入进行检查，可以看出本函数的输入应当是长度为6的字符串。如果检查通过，则进入下面的循环部分。
### part2:
```
0x4010d2 <phase_5+112>  mov    $0x0,%eax                                                                                                                                                              
0x4010d7 <phase_5+117>  jmp    0x40108b <phase_5+41>  
0x40108b <phase_5+41>   movzbl (%rbx,%rax,1),%ecx                                                                                                                                                     
0x40108f <phase_5+45>   mov    %cl,(%rsp)                                                                                                                                                             
0x401092 <phase_5+48>   mov    (%rsp),%rdx                                                                                                                                                            
0x401096 <phase_5+52>   and    $0xf,%edx                                                                                                                                                              
0x401099 <phase_5+55>   movzbl 0x4024b0(%rdx),%edx                                                                                                                                                    
0x4010a0 <phase_5+62>   mov    %dl,0x10(%rsp,%rax,1)                                                                                                                                                  
0x4010a4 <phase_5+66>   add    $0x1,%rax                                                                                                                                                              
0x4010a8 <phase_5+70>   cmp    $0x6,%rax                                                                                                                                                              
0x4010ac <phase_5+74>   jne    0x40108b <phase_5+41>
```
为了方便阅读，在这里对语句的顺序进行了调整，可以看出这是一个循环语句，在循环中将 `input` 指向的字符串拷贝至 `$rsp` 处，并将字符串中各字符与 `0xf` 做and运算，将运算结果作为地址偏移量，并将 `0x4024b0` 加该偏移量处的字符逐个拷贝至 `$rsp+16` 处。  
利用 `x /1s 0x4024b0` 可以查看存放在该地址处的字符串：  

```
0x4024b0 <array.3449>:  "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
```
### part3
```
0x4010ae <phase_5+76>   movb   $0x0,0x16(%rsp)                                                                                                                                                        
0x4010b3 <phase_5+81>   mov    $0x40245e,%esi                                                                                                                                                         
0x4010b8 <phase_5+86>   lea    0x10(%rsp),%rdi                                                                                                                                                        
0x4010bd <phase_5+91>   callq  0x401338 <strings_not_equal>                                                                                                                                           
0x4010c2 <phase_5+96>   test   %eax,%eax                                                                                                                                                              
0x4010c4 <phase_5+98>   je     0x4010d9 <phase_5+119>                                                                                                                                                 
0x4010c6 <phase_5+100>  callq  0x40143a <explode_bomb>                                                                                                                                                
0x4010cb <phase_5+105>  nopl   0x0(%rax,%rax,1)                                                                                                                                                       
0x4010d0 <phase_5+110>  jmp    0x4010d9 <phase_5+119>
```
该部分将 `$rsp+16` 处的字符串与 `0x40245e` 处的字符串做比较，如果相同则通过函数，利用 `x /1s 0x40245e`以查看该字符串，得到以下输出：  
```
0x40245e:       "flyers"
```
因此，可以看出本问题的答案即为寻找6个字符，使得6个字符的ASCII码的低4位分别对应于 `"flyers"` 六个字符在 `0x4024b0` 处的偏移量：9 15 14 5 6 7，答案并不唯一，`"IONEFG"`可以是本问题的答案之一。
## phase_6
该问题的汇编代码很长，但是我们只需要将其拆分成几个独立的部分，逐个观察其对输入字符串性质的限制即可破解本题。
### part1:
```
0x401100 <phase_6+12>   mov    %rsp,%r13                                                                                                                                                              
0x401103 <phase_6+15>   mov    %rsp,%rsi  
0x401106 <phase_6+18>   callq  0x40145c <read_six_numbers>                                                                                                                                            
0x40110b <phase_6+23>   mov    %rsp,%r14                                                                                                                                                              
0x40110e <phase_6+26>   mov    $0x0,%r12d                                                                                                                                                             
0x401114 <phase_6+32>   mov    %r13,%rbp                                                                                                                                                              
0x401117 <phase_6+35>   mov    0x0(%r13),%eax                                                                                                                                                         
0x40111b <phase_6+39>   sub    $0x1,%eax                                                                                                                                                              
0x40111e <phase_6+42>   cmp    $0x5,%eax                                                                                                                                                              
0x401121 <phase_6+45>   jbe    0x401128 <phase_6+52>                                                                                                                                                  
0x401123 <phase_6+47>   callq  0x40143a <explode_bomb>                                                                                                                                                
0x401128 <phase_6+52>   add    $0x1,%r12d                                                                                                                                                             
0x40112c <phase_6+56>   cmp    $0x6,%r12d                                                                                                                                                             
0x401130 <phase_6+60>   je     0x401153 <phase_6+95>                                                                                                                                                  
0x401132 <phase_6+62>   mov    %r12d,%ebx                                                                                                                                                             
0x401135 <phase_6+65>   movslq %ebx,%rax                                                                                                                                                              
0x401138 <phase_6+68>   mov    (%rsp,%rax,4),%eax
0x40113b <phase_6+71>   cmp    %eax,0x0(%rbp)                                                                                                                                                         
0x40113e <phase_6+74>   jne    0x401145 <phase_6+81>                                                                                                                                                  
0x401140 <phase_6+76>   callq  0x40143a <explode_bomb>                                                                                                                                                
0x401145 <phase_6+81>   add    $0x1,%ebx                                                                                                                                                              
0x401148 <phase_6+84>   cmp    $0x5,%ebx                                                                                                                                                              
0x40114b <phase_6+87>   jle    0x401135 <phase_6+65>                                                                                                                                                  
0x40114d <phase_6+89>   add    $0x4,%r13                                                                                                                                                              
0x401151 <phase_6+93>   jmp    0x401114 <phase_6+32>                                                                                                                                                  
0x401153 <phase_6+95>   lea    0x18(%rsp),%rsi
```
函数的第一部分是一个双循环语句，虽然代码的字段比较长，但其性质比较简单，即输入六个数，当其存在重复元素或元素大于5时触发炸弹函数，可以转换为等价的C语句：  
```
int num[6];
read_six_numbers(input,num);
for(int i=0;i<6;++i){
    if(num[i] > 5) explode_bomb();
    int j = i+1;
    while(j < 6){
	if(num[j] == num[i]) explode_bomb();
	++j;
    }
}
```
### part2:
```
0x401153 <phase_6+95>   lea    0x18(%rsp),%rsi                                                                                                                                                        
0x401158 <phase_6+100>  mov    %r14,%rax                                                                                                                                                              
0x40115b <phase_6+103>  mov    $0x7,%ecx                                                                                                                                                              
0x401160 <phase_6+108>  mov    %ecx,%edx                                                                                                                                                              
0x401162 <phase_6+110>  sub    (%rax),%edx                                                                                                                                                            
0x401164 <phase_6+112>  mov    %edx,(%rax)                                                                                                                                                            
0x401166 <phase_6+114>  add    $0x4,%rax                                                                                                                                                              
0x40116a <phase_6+118>  cmp    %rsi,%rax                                                                                                                                                              
0x40116d <phase_6+121>  jne    0x401160 <phase_6+108>
```
该部分的功能是将之前输入的6个数对7取补，可以转换为以下的C语句：  
```
for(int i=0;i<6;i++)
	num[i] = 7 - num[i];
```
### part3:
```
0x40116f <phase_6+123>  mov    $0x0,%esi                                                                                                                                                              
0x401174 <phase_6+128>  jmp    0x401197 <phase_6+163>                                                                                                                                                 
0x401176 <phase_6+130>  mov    0x8(%rdx),%rdx                                                                                                                                                         
0x40117a <phase_6+134>  add    $0x1,%eax                                                                                                                                                              
0x40117d <phase_6+137>  cmp    %ecx,%eax                                                                                                                                                              
0x40117f <phase_6+139>  jne    0x401176 <phase_6+130>                                                                                                                                                 
0x401181 <phase_6+141>  jmp    0x401188 <phase_6+148>                                                                                                                                                 
0x401183 <phase_6+143>  mov    $0x6032d0,%edx                                                                                                                                                         
0x401188 <phase_6+148>  mov    %rdx,0x20(%rsp,%rsi,2)                                                                                                                                                 
0x40118d <phase_6+153>  add    $0x4,%rsi                                                                                                                                                              
0x401191 <phase_6+157>  cmp    $0x18,%rsi                                                                                                                                                             
0x401195 <phase_6+161>  je     0x4011ab <phase_6+183>                                                                                                                                                 
0x401197 <phase_6+163>  mov    (%rsp,%rsi,1),%ecx                                                                                                                                                     
0x40119a <phase_6+166>  cmp    $0x1,%ecx                                                                                                                                                              
0x40119d <phase_6+169>  jle    0x401183 <phase_6+143>                                                                                                                                                
0x40119f <phase_6+171>  mov    $0x1,%eax                                                                                                                                                              
0x4011a4 <phase_6+176>  mov    $0x6032d0,%edx                                                                                                                                                         
0x4011a9 <phase_6+181>  jmp    0x401176 <phase_6+130>                                                                                                                     
```
该部分的性质比较复杂，首先我们利用 `x /12xg 0x6032d0` 观察语句中出现的常量地址中的内容： 
```
0x6032d0 <node1>:       0x000000010000014c      0x00000000006032e0
0x6032e0 <node2>:       0x00000002000000a8      0x00000000006032f0
0x6032f0 <node3>:       0x000000030000039c      0x0000000000603300
0x603300 <node4>:       0x00000004000002b3      0x0000000000603310
0x603310 <node5>:       0x00000005000001dd      0x0000000000603320
0x603320 <node6>:       0x00000006000001bb      0x0000000000000000
```
可以看出在0x6032d0 - 0x603310的相邻16的六个地址（设其为addr）中，具有 `*(addr+8) == addr+16` 的性质。根据此性质即可理解 `<phase_6+130>`至`<phase_6+139>`的内层循环语句，其将rdx寄存器中的值加上了 `($ecx-1)*16`。据此即可将本部分转换为等价的C语句：  
``` 
int* addrs[6];	
for(int i=0;i<6;i++){
    int c = num[i];
    int addr = 0x6032d0;
    if(c > 1) addr += (c-1)*16;
    addrs[i] = addr;
}
```
可以看出，该部分将 `$rsp` 处存放的6个整数作为偏移量，在栈帧处存放了6个指针数据。
### part4:
```
0x4011ab <phase_6+183>  mov    0x20(%rsp),%rbx                                                                                                                                                        
0x4011b0 <phase_6+188>  lea    0x28(%rsp),%rax                                                                                                                                                        
0x4011b5 <phase_6+193>  lea    0x50(%rsp),%rsi                                                                                                                                                        
0x4011ba <phase_6+198>  mov    %rbx,%rcx                                                                                                                                                              
0x4011bd <phase_6+201>  mov    (%rax),%rdx                                                                                                                                                            
0x4011c0 <phase_6+204>  mov    %rdx,0x8(%rcx)                                                                                                                                                         
0x4011c4 <phase_6+208>  add    $0x8,%rax                                                                                                                                                              
0x4011c8 <phase_6+212>  cmp    %rsi,%rax                                                                                                                                                              
0x4011cb <phase_6+215>  je     0x4011d2 <phase_6+222>                                                                                                                                                 
0x4011cd <phase_6+217>  mov    %rdx,%rcx                                                                                                                                                              
0x4011d0 <phase_6+220>  jmp    0x4011bd <phase_6+201>
0x4011d2 <phase_6+222>  movq   $0x0,0x8(%rdx)
```
该部分将上部分代码中存放的指针数组中前5个指针地址加8处存放数组中的下一个指针值，可以转化为下面的C语句方便理解：
```  
for(int i=0;i<5;i++){
    *(addrs[i]+8) = addrs[i+1];
```
当函数的输入参数为`"4 3 2 1 6 5"`时，在执行完该部分语句后再次输入 `x /12xg 0x6032d0` 以及查看该处存放内容的变化情况：  
```
0x6032d0 <node1>:       0x000000010000014c      0x00000000006032e0
0x6032e0 <node2>:       0x00000002000000a8      0x00000000006032f0
0x6032f0 <node3>:       0x000000030000039c      0x0000000000603300
0x603300 <node4>:       0x00000004000002b3      0x0000000000603310
0x603310 <node5>:       0x00000005000001dd      0x0000000000603320
0x603320 <node6>:       0x00000006000001bb      0x00000000006032d0
```
输入 `x /10xg $rsp` 查看栈帧中存放的两个数组：  
```
0x7fffffffdf30: 0x0000000400000003      0x0000000600000005
0x7fffffffdf40: 0x0000000200000001      0x0000000000000000
0x7fffffffdf50: 0x00000000006032f0      0x0000000000603300
0x7fffffffdf60: 0x0000000000603310      0x0000000000603320
0x7fffffffdf70: 0x00000000006032d0      0x00000000006032e0
```
可以看出内存中的变化情况满足之前所作的假设。
### part5:
```
0x4011da <phase_6+230>  mov    $0x5,%ebp                                                                                                                                                              
0x4011df <phase_6+235>  mov    0x8(%rbx),%rax                                                                                                                                                         
0x4011e3 <phase_6+239>  mov    (%rax),%eax                                                                                                                                                            
0x4011e5 <phase_6+241>  cmp    %eax,(%rbx)                                                                                                                                                            
0x4011e7 <phase_6+243>  jge    0x4011ee <phase_6+250>                                                                                                                                                 
0x4011e9 <phase_6+245>  callq  0x40143a <explode_bomb>                                                                                                                                                
0x4011ee <phase_6+250>  mov    0x8(%rbx),%rbx                                                                                                                                                         
0x4011f2 <phase_6+254>  sub    $0x1,%ebp                                                                                                                                                              
0x4011f5 <phase_6+257>  jne    0x4011df <phase_6+235>
```
将其转化为等价的C语句：  
```
for(int i=0;i<5;i++){
    if(*(addrs[i]) <= *(addrs[i]+8))
	explode_bomb();
}
```
结合part4中的内容，每个 `addrs[i]+8` 中存放的地址即为 `addrs[i+1]`，因此，为了不触发炸弹函数，指针数组中存放的相邻两个地址之间的值应当满足**前一个地址处的值大于后一个地址处的值**。  
在这里按照地址中存储值的大小进行排序，可以得到 `*0x6032f0 > *0x603300 > *0x603310 > *0x603320 > *0x6032d0 > *0x6032e0`。根据part1至part3中的分析，指针数组中存放的指针与输入的整数有如下关系 `addrs[i] == 0x6032d0 + (7-num[i])*16`。据此即可得到本问题的答案:`"4 3 2 1 6 5"`。                                                                                             
