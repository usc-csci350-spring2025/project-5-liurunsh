echo "Running test12..."
echo

out=$(sudo ./main <tests/input12.txt >outputs/output12.txt)
output=$(sort outputs/output12.txt)
expected_output='a 15
able 1
about 2
above 1
all 1
although 1
always 1
an 1
and 8
any 1
arches 1
arm 1
armourlike 1
as 1
at 2
back 2
be 1
because 1
bed 1
bedding 1
before 1
began 1
belly 1
between 1
bit 1
boa 1
brown 1
but 1
by 1
collection 1
compared 1
could 2
couldnt 1
cover 1
covered 1
cut 1
divided 1
do 1
domed 1
dream 1
dreams 1
drops 1
dull 2
eyes 1
familiar 1
feel 2
felt 1
fitted 1
floundering 1
forget 1
found 1
four 1
frame 1
from 1
fur 3
get 1
gilded 1
gregor 2
had 2
happened 1
hard 1
hardly 1
hat 1
have 2
he 17
head 1
heard 1
heavy 1
helplessly 1
her 1
him 2
himself 2
his 10
hitting 1
horrible 1
housed 1
how 1
however 1
human 1
hundred 1
hung 1
i 1
if 2
illustrated 1
in 3
into 3
it 5
its 1
lady 1
lay 3
legs 2
lifted 1
little 3
longer 1
look 2
looked 1
lower 1
made 1
magazine 1
many 1
me 1
mild 1
moment 1
morning 1
muff 1
must 1
never 1
nice 1
nonsense 1
of 6
off 1
on 3
one 1
only 1
onto 1
out 4
pain 1
pane 1
peacefully 1
picture 1
pitifully 1
position 1
present 1
proper 1
quite 1
rain 1
raising 1
ready 1
recently 1
rest 1
right 2
rolled 1
room 2
sad 1
salesmanand 1
samples 1
samsa 1
sat 1
sections 1
see 1
seemed 1
showed 1
shut 1
size 1
sleep 1
sleeping 1
slide 1
slightly 1
small 1
so 1
something 1
spread 1
state 1
stiff 1
stopped 1
tablesamsa 1
textile 1
that 6
the 10
then 1
there 2
thin 1
this 1
thought 2
threw 1
times 1
to 9
too 1
towards 1
transformed 1
travelling 1
tried 1
troubled 1
turned 1
unable 1
upright 1
used 1
vermin 1
viewer 1
walls 1
was 6
wasnt 1
waved 1
weather 1
whats 1
when 2
where 1
which 1
who 1
whole 1
window 1
with 2
woke 1
wouldnt 1'

if [ $? -eq 0 ] ; then
  echo "Pass: Program exited zero"
else
  echo "Fail: Program did not exit zero"
  exit 1
fi

if [ "$output" == "$expected_output" ] ; then
  echo "Pass: Output is correct"
else
  echo "Expected '$expected_output' but got: $output"
  exit 1
fi

echo
echo "Test12 passed."

exit 0
