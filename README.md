# Towers of Hanoi 

## Kosmos CP1

### Assembly Version (for readability) 

```
FUNCTION MoveTower(disk, source, dest, spare):
IF disk == 0, THEN:
    move disk from source to dest
ELSE:
    MoveTower(disk - 1, source, spare, dest)   // Step 1 above
    move disk from source to dest              // Step 2 above
    MoveTower(disk - 1, spare, dest, source)   // Step 3 above
END IF 

-------------------------------------------------------

140 aux 1 
141 aux 2 
142 aux 3
143 simple return 1
144 simple return 2 

145 disk nr. 
146 source stack pointer
147 spare  stack pointer
148 dest   stack pointer
149 return stack pointer

-------------------------------------------------------
# value stacks
# 3 value stack pointers: source, spare, dest
# disk nr. is maintained by the code (+1, -1) 

200 -> 239 (13 entries) 

-------------------------------------------------------
# return stack
# 1 return stack pointer

240 -> 255 (13+ entries) 

-------------------------------------------------------
hanoi: 

#
# prepare stack pointers 
#

000 ako 04.<source stack start>   # 200 
001 abs 06.<source stack pointer> # 146

002 ako 04.<spare stack start>    # 201
003 abs 06.<spare stack pointer>  # 147

004 ako 04.<dest stack start>     # 202
005 abs 06.<dest stack pointer>   # 148  

006 ako 04.<return stack start>   # 240 - return stack starts here 
007 abs 06.<return stack pointer> # 149 

#
# prepare stacks (load values into value frame, push return address) 
# for main call
# 

008 ako 04.003 
009 abs 06.<disk number>          # number of disks - no stack 

010 ako 04.001                    # source peg number = 1 
011 ais 20.<source stack pointer> # source peg number -> stack frame 

012 ako 04.002                    # spare peg number = 2
013 ais 20.<spare stack pointer>  # spare peg number -> stack frame 

014 ako 04.003                    # dest peg number = 3 
015 ais 20.<dest stack pointer>   # dest peg number -> stack frame 

016 ako 04.<end>                  # continuation address after recursive call 
017 ais 20.<return stack pointer> # push return address onto return stack

#
# toplevel call 
# 
 
018 spu 09.<move_tower>  

#
# returned from toplevel call  
# 

end:

019 hlt 01.00  

-------------------------------------------------------

# function movetower(disk, source, dest, spare):

move_tower: 

020 lda 05.<disk nr>  
021 vgl 10.<1 const>     # one?
022 spb 11.<bottom case> # branch if zero 

#
# disk > 0: inductive case 
#

rec_case:

# prepare the first recursive call:
# movetower(disk - 1, source, spare, dest)
# prepare n-1 disk number

# source <- source
# dest <- spare
# spare <- dest

# save old stack frame values into aux registers

023 ako 04.<label 0>  
024 abs 06.<simple return 1>
025 spu 09.<save and push>

label 0:

# source <- source, store into stack frame

026 lda 05.<aux 1>
027 ais 20.<source stack pointer>

# dest <-spare, store into stack frame

028 lda 05.<aux 3>
029 ais 20.<dest stack pointer>

# spare <- dest, store into stack frame

030 lda 05.<aux 2>
031 ais 20.<spare stack pointer>

# stack frame ready and filled,
# now push return address onto return stack

032 ako 04.<label 1>               # push return address... 
033 ais 20.<return stack pointer>  # ...onto return stack 

# both value and return stack prepared, 
# do the recursive call! 

034 spu 09.<move tower>   

label 1:

# restore stacks and disk nr. 

035 ako 04.<label 2> 
036 abs 05.<simple return 1> 
037 spu 09.<pop and restore> 

label 2:

# now output the "move disk <disk nr> from <source> to <dest> 

038 ako 04.<label 3> 
039 abs 06.<simple return 1>
040 spu 09.<move one disk>  

label 3:

# prepare the second recursive call: 
# movetower(disk - 1, spare, dest, source)
# a copy of the first call, but peg name shuffling differs

# source <- spare
# dest <- dest
# spare <- source 

041 ako 04.<label 4> 
042 abs 06.<simple return 1>
043 spu 09.<save and push>

label 4:

# source <- dest, store into stack frame   

044 lda 05.<aux 3>
045 ais 20.<source stack pointer>

# dest <- dest, store into stack frame 

046 lda 05.<aux 2>
047 ais 20.<dest stack pointer>

# spare <- source, store into stack frame 

048 lda 05.<aux 1>
049 ais 20.<spare stack pointer>

# do the second recursive call! 

050 ako 04.<label 5>               # push return address... 
051 ais 20.<return stack pointer>  # ...onto return stack 

052 spu 09.<move tower>   

label 5:

# restore stacks and disk nr. 

053 ako 04.<label 6> 
054 abs 05.<simple return 1> 
055 spu 09.<pop and restore>

label 6: 

# return to previous incarnation level 

056 spu 09.<return block>

#
# bottom-case: move disk from source to dest
# 
  
# make move_one_disk call

057 ako 04.<label 7> 
058 abs 06.<simple return 1>
059 spu 09.<move one disk>  

label 7:

# return to previous incarnation level 

060 spu 09.<return block>  

-------------------------------------------------------

#
# subroutine: move one disk from source to dest 
#

move_one_disk:

# show separator 1 

061 ako 04.<show_disk>        # register return address... 
062 abs 06.<simple return 2>   
063 lda 05.<sep_const_1>      # show separator 1 
064 spu 09.<disp>  

# show disk number 

show_disk:

065 ako 04.<show_source>           
066 abs 06.<simple return 2>  
067 lda 05.<disk nr>    
068 spu 09.<disp>  

# show source peg 

show_source:

069 ako 04.<show_dest>             
070 abs 06.<simple return 2>  
071 lia 19.<source stack pointer>  
072 spu 09.<disp>  

# show dest peg 

show_dest:

073 ako 04.<return_disp>            
074 abs 06.<simple return 2>  
075 lia 19.<dest stack pointer>    
076 spu 09.<disp>  

# return 

return_disp:

077 siu 21.<simple return 1> 

-------------------------------------------------------

#
# subroutine: display accu with separator and delay 
# 

disp:

078 anz 02.000  # display accu value 
 
079 vzg 03.255  # delay of 2 seconds 
080 vzg 03.255
081 vzg 03.255
082 vzg 03.255
083 vzg 03.255
084 vzg 03.255
085 vzg 03.255
086 vzg 03.255

# show separator 2

087 lda 05.<sep_const_2>  
088 anz 02.000
 
089 vzg 03.255
090 vzg 03.255
091 vzg 03.255
092 vzg 03.255
093 vzg 03.255
094 vzg 03.255
095 vzg 03.255
096 vzg 03.255

097 SIU 21.<simple return 2> 

-------------------------------------------------------

#
# sub-routine to decr. disk nr., save current values to aux 
# and create new stack frame for values and return stack
#

save_and_push: 

098 lda 05.<disk nr> # load disk 
099 sub 08.<1 const> # sub 1
100 abs 06.<disk nr> # save 

101 lia 19.<source stack pointer> # save source
102 abs 06.<aux 1>     

103 lia 19.<dest stack pointer>	  # save dest
104 abs 06.<aux 2>     

105 lia 19.<spare stack pointer>  # save spare 
106 abs 06.<aux 3>

# create new stack frame for source, spare, dest 

107 lda 05.<source stack pointer> 
108 add 07.<3 const>
109 abs 06.<source stack pointer>

110 lda 05.<dest stack pointer> 
111 add 07.<3 const>
112 abs 06.<dest stack pointer>

113 lda 05.<spare stack pointer>
114 add 07.<3 const>
115 abs 06.<spare stack pointer>

# create new return stack entry 
 
116 lda 05.<return stack pointer> 
117 add 07.<1 const>
118 abs 06.<return stack pointer>

119 siu 21.<simple return 1> 

-------------------------------------------------------

#
# sub-routine to incr. disk nr., 
# and pop stack frame for values and return stack 
#

pop_and_restore: 

120 lda 05.<disk nr> # load disk 
121 add 07.<1 const> # add 1
122 abs 06.<disk nr> # save 

123 lda 05.<source stack pointer> 
124 sub 08.<3 const>
125 abs 06.<source stack pointer>

126 lda 05.<dest stack pointer> 
127 sub 08.<3 const>
128 abs 06.<dest stack pointer>

129 lda 05.<spare stack pointer>
130 sub 08.<3 const>
131 abs 06.<spare stack pointer>

132 lda 05.<return stack pointer> 
133 sub 08.<1 const>
134 abs 06.<return stack pointer>

135 siu 21.<simple return 1>

-------------------------------------------------------

return block: 

136 lia 19.<return stack pointer> 
137 abs 06.<simple return 1>
138 siu 21.<simple return 1>

-------------------------------------------------------

```

### Linked Version (Machine Language for the CP1) 

```
FUNCTION MoveTower(disk, source, dest, spare):
IF disk == 0, THEN:
    move disk from source to dest
ELSE:
    MoveTower(disk - 1, source, spare, dest)   // Step 1 above
    move disk from source to dest              // Step 2 above
    MoveTower(disk - 1, spare, dest, source)   // Step 3 above
END IF 

-------------------------------------------------------

140 aux 1 
141 aux 2 
142 aux 3
143 simple return 1
144 simple return 2 

145 disk nr. 
146 source stack pointer
147 spare  stack pointer
148 dest   stack pointer
149 return stack pointer

150 00.001 const 1
151 00.003 const 3
152 11.111 sep 1
153 22.222 sep 2 

-------------------------------------------------------
# value stacks
# 3 value stack pointers: source, spare, dest
# disk nr. is maintained by the code (+1, -1) 

200 -> 239 (13 entries) 

-------------------------------------------------------
# return stack
# 1 return stack pointer

240 -> 255 (13+ entries) 

-------------------------------------------------------
hanoi: 

#
# prepare stack pointers 
#

000 ako 04.200 # value stack start 
001 abs 06.146 # source * 

002 ako 04.201 
003 abs 06.147 # spare * 

004 ako 04.202 
005 abs 06.148 # dest * 

006 ako 04.240 # return stack start 
007 abs 06.149 # return stack * 

#
# prepare stacks (load values into value frame, push return address) 
# for main call
# 

008 ako 04.003 # disk nr 
009 abs 06.145 # number of disks - no stack 

010 ako 04.001 # source peg number = 1 
011 ais 20.146 # source peg number -> stack frame 

012 ako 04.002 # spare peg number = 2
013 ais 20.147 # spare peg number -> stack frame 

014 ako 04.003 # dest peg number = 3 
015 ais 20.148 # dest peg number -> stack frame 

016 ako 04.019 # continuation address after recursive call 
017 ais 20.149 # push return address onto return stack

#
# toplevel call 
# 
 
018 spu 09.020 

#
# returned from toplevel call  
# 

end:

019 hlt 01.00  

-------------------------------------------------------

# function movetower(disk, source, dest, spare):

move_tower: 

020 lda 05.145  
021 vgl 10.150 # one?
022 spb 11.057 # branch if 

#
# disk > 0: inductive case 
#

rec_case:

# prepare the first recursive call:
# movetower(disk - 1, source, spare, dest)
# prepare n-1 disk number

# source <- source
# dest <- spare
# spare <- dest

# save old stack frame values into aux registers

023 ako 04.026 # label_0 
024 abs 06.143 # simple return 1 
025 spu 09.098 # save_and_push 

label_0:

# source <- source, store into stack frame

026 lda 05.140 # aux 1
027 ais 20.146 # -> *source 

# dest <-spare, store into stack frame

028 lda 05.142 # aux 3
029 ais 20.148 # -> *dest

# spare <- dest, store into stack frame

030 lda 05.141 # aux 2
031 ais 20.147 # -> *spare 

# stack frame ready and filled,
# now push return address onto return stack

032 ako 04.035 # push return address... 
033 ais 20.149 # ...onto return stack 

# both value and return stack prepared, 
# do the recursive call! 

034 spu 09.020 

label_1:

# returned from recursive call, pop value stacks

035 ako 04.038 # label_2 
036 abs 06.143 # simple return 1 
037 spu 09.120 # pop_and_restore 

label_2:

038 ako 04.041 # label 3 
039 abs 06.143 # simple return 1 
040 spu 09.061 # move_one_disk 

label_3:

# prepare the second recursive call: 
# movetower(disk - 1, spare, dest, source)
# a copy of the first call, but peg name shuffling differs

# source <- spare
# dest <- dest
# spare <- source 

041 ako 04.044 # label_4 
042 abs 06.143 # simple return 1 
043 spu 09.098 # save_and_push

label_4:

# source <- dest, store into stack frame   

044 lda 05.142
045 ais 20.146

# dest <- dest, store into stack frame 

046 lda 05.141
047 ais 20.148

# spare <- source, store into stack frame 

048 lda 05.140
049 ais 20.147

# do the second recursive call! 

050 ako 04.053 # label_5 
051 ais 20.149 # ...onto return stack 

052 spu 09.020

label_5:

# pop value stacks

053 ako 04.056 # label_6 
054 abs 06.143 # simple return 1 
055 spu 09.120 # pop and restore 

label_6: 

# return to previous incarnation level 

056 spu 09.136 # return block 

#
# bottom-case: move disk from source to dest
# 
  
# make move_one_disk call

057 ako 04.060 # label_7 
058 abs 06.143 # simple return 1
059 spu 09.061 # move one disk output

label_7:

# return 

060 spu 09.136 # return block 

-------------------------------------------------------

#
# subroutine: move one disk from source to dest 
#

move_one_disk:

# show separator 1 

061 ako 04.065 # show_disk 
062 abs 06.144 # simple return 2! 
063 lda 05.152 # show separator 1 
064 spu 09.078 # disp 

# show disk number 

show_disk:

065 ako 04.069 # show_source 
066 abs 06.144 # simple return 2
067 lda 05.145 # disk nr. 
068 spu 09.078 # disp 

# show source peg 

show_source:

069 ako 04.073 # show_dest
070 abs 06.144 # simple return 2
071 lia 19.146 # source* 
072 spu 09.078 # disp 

# show dest peg 

show_dest:

073 ako 04.077 # return_disp 
074 abs 06.144 # simple return 2
075 lia 19.148 # target*
076 spu 09.078 # disp 

# return 

return_disp:

077 siu 21.143 # simple return 1

-------------------------------------------------------

#
# subroutine: display accu with separator and delay 
# 

disp:

078 anz 02.000 # display accu value 
 
079 vzg 03.255 # delay of 2 seconds 
080 vzg 03.255
081 vzg 03.255
082 vzg 03.255
083 vzg 03.255
084 vzg 03.255
085 vzg 03.255
086 vzg 03.255

# show separator 2

087 lda 05.153 
088 anz 02.000
 
089 vzg 03.255
090 vzg 03.255
091 vzg 03.255
092 vzg 03.255
093 vzg 03.255
094 vzg 03.255
095 vzg 03.255
096 vzg 03.255

097 siu 21.144 # simple return 2 

-------------------------------------------------------

#
# sub-routine to decr. disk nr., save current values to aux 
# and create new stack frame for values and return stack
#

save_and_push: 

098 lda 05.145 # load disk 
099 sub 08.150 # sub 1
100 abs 06.145 # save 

101 lia 19.146 # save source
102 abs 06.140     

103 lia 19.148 # save dest
104 abs 06.141     

105 lia 19.147 # save spare 
106 abs 06.142

# create new stack frame for source, spare, dest 

107 lda 05.146 
108 add 07.151
109 abs 06.146

110 lda 05.148 
111 add 07.151
112 abs 06.148

113 lda 05.147
114 add 07.151
115 abs 06.147

# prepare new return stack frame

116 lda 05.149 
117 add 07.150
118 abs 06.149

119 siu 21.143 # simple return 1 

-------------------------------------------------------

#
# sub-routine to incr. disk nr., 
# and pop stack frame for values and return stack 
#

pop_and_restore: 

120 lda 05.145 # load disk 
121 add 07.150 # add 1
122 abs 06.145 # save 

123 lda 05.146 
124 sub 08.151
125 abs 06.146

126 lda 05.148 
127 sub 08.151
128 abs 06.148

129 lda 05.147
130 sub 08.151
131 abs 06.147

132 lda 05.149 
133 sub 08.150
134 abs 06.149

135 siu 21.143 # simple return 1

-------------------------------------------------------

return block: 

136 lia 19.149 
137 abs 06.143 
138 siu 21.143

-------------------------------------------------------
``` 

## Busch Microtronic 

```
WORKS, UP TO 4 DISKS!
THEN OUT OF VALUE STACK !

-----------------------

FUNCTION MoveTower(disk, source, dest, spare):
IF disk == 0, THEN:
    move disk from source to dest
ELSE:
    MoveTower(disk - 1, source, spare, dest)   // Step 1 above
    move disk from source to dest              // Step 2 above
    MoveTower(disk - 1, spare, dest, source)   // Step 3 above
END IF 

----------------

Microtronic: emulate recursion

0..7 = registers for program

----------------- 

f...8 = return stack

2nd bank:

f...0 = value stack,
upper half can be swapped in for communication with 0 - 7 normal bank 

-----------------

00 f08
01 f0e
02 f1f
03 fff
04 f02
05 1ae # A -> source
06 1cd # C -> dest
07 1bc # B -> spare 
08 f0e

09 11f # move "return marker 1" top top of return stack (F); marker 1 -> 0b
0a c0d # goto hanoi(n, source, dest, spare); "recursive" call 

# end 

0b f60 # stop 
0c f00

### hanoi(n, source, dest, spare) @0d 

## f = 01?

0d f0e # swap in value stack 
0e 91f 
0f D15 

## = 01; base case 

10 f3d
11 ff0 # this doesn't destroy the value stack, as only 8-f has been swapped in!
12 f02 
13 f0e # swap back normal registers 
14 cb0 # goto jumpblock  ; return

## > 01; rec call and add 

# create 4 value stack entries

15 f0e # restore normal registers 
16 b70 # shift value stack down
17 b70 
18 b70 
19 b70 

# swap in upper bank to put values on value stack
# for first rec. call:
# FUNCTION MoveTower(disk, source, dest, spare):
# MoveTower(disk - 1, source, spare, dest)   // Step 1 above

1a f0e # swap in upper values bank 
1b 0bf # copy "n" value 
1c 71f # sub 1 from n -> n-1 
1d 0ae # source -> source
1e 09c # dest -> spare 
1f 08d # spare -> dest 
20 f0e 

21 b50 # shift return stack down 
22 12f -> move "return marker 2" top top of return stack (F); "marker 2 -> 24"

# parameters were prepared, do the call

23 c0d # goto sum(n);  recursive call

24 b60 # shift return stack up 
25 b90 # shift value stack up
26 b90
27 b90
28 b90

# output
# move disk from source to dest              // Step 2 above

29 f0e
2a f3d 
2b ff0
2c f02
2d f0e 

#
# prepare 2nd recursive call 
# 

2e b70 # shift value stack down
2f b70 
30 b70 
31 b70 

# swap in upper bank to put values on value stack
# for second rec. call:
# FUNCTION MoveTower(disk, source, dest, spare):
# MoveTower(disk - 1, spare, dest, source)   // Step 3 above

32 f0e # swap in upper values bank 
33 0bf # copy "n" value 
34 71f # sub 1 from n -> n-1 
35 08e # spare -> source
36 09d # dest -> dest  
37 0ac # source -> spare  
38 f0e 

39 b50 # shift return stack down 
3a 13f -> move "return marker 3" top top of return stack (F); "marker 3 -> 3c"

# parameters were prepared, do the call

3b c0d # goto sum(n);  recursive call

3c b60 # shift return stack up 
3d b90 # shift value stack up
3e b90
3f b90
40 b90

41 cb0 # goto jumpblock  ; return 

###

return stack shift down: 

50:

50 098
51 0a9
52 0ba
53 0cb
54 0dc
55 0ed
56 0fe
57 fo7 

#### 

return stack shift up: 

60:

60 0ef
61 0de
62 0cd 
63 0bc
64 0ab
65 09a
66 089 
67 fo7 

####

value stack shift down: 

70:

70 f0d
71 f0e 
72 010
73 021
74 032
75 043
76 054
77 065
78 076
79 087
7a 098
7b 0a9
7c 0ba
7d 0cb
7e 0dc
7f 0ed
80 0fe 
81 f0e
82 f0d 
83 f07 # ret 

#### 

90:

value stack shift up: 

90 f0d
91 f0e 
92 0ef
93 0de
94 0cd
95 0bc
96 0ab
97 09a
98 089
99 078
9a 067
9b 056
9c 045
9d 034
9e 023
9f 012
a0 001
a1 f0e
a2 f0d 
a3 f07 # ret 

###

jump block

b0:

b0: 91f # marker 1 ?
b1: E0b # goto 0A
b2: 92f # marker 2 ?
b3: E24 # goto 24
b4: 93f # marker 3 ?
b5: E3c # goto 3c
b6: FOO # error

---------------

Test run with 3 disks: 

A B C
-----
1
2
3
-----

2
3   1
-----


3 2 1
-----

  1
3 2
-----

  1
  2 3
-----


1 2 3
-----

    2
1   3
-----
    1
    2
    3
-----
  
7 Steps

``` 
