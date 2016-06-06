#!/usr/bin/perl

system ('rm 50BatWC.txt');
$init = 0.50;

for($fn=1; $fn <=30; $fn = $fn + 1)
 {

open (INFILE, "$fn.txt");
open (OUTFILE, '>input.txt');
while ($line1=<INFILE>)
{
        print OUTFILE "$line1";
}
 close (INFILE);
 close (OUTFILE);

 system ('gcc -o nano nanogrid5.c');
 system ("nano 9072000 $init 1 0 1 0.1 > out");
 
open (INFILE, 'out');
open (OUTFILE, '>>50BatWC.txt');
 
 while ($line1=<INFILE>)
{
        print OUTFILE "$line1";
}

 close (INFILE);
 close (OUTFILE);
 
 open (INFILE, 'out');
 open (OUTFILE, '>Step1.txt');

 while ($line1=<INFILE>)
{
	 if($line1 =~ /Flag/)
		{
			print OUTFILE "$line1";
		}	
}

 close (INFILE);
  close (OUTFILE);

 open (INFILE, 'Step1.txt');
while ($line1=<INFILE>)
{
	@comp1 = split(' ',$line1);
	$init = @comp1[3];
  print "$init\n";
}
 close (INFILE);



}

