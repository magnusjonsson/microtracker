make &&
./song2abc -T "Utonal Seventh" -C "Magnus Jonsson" utonal-seventh.song > us.abp &&
microabc -i- -S -Pus.abp > us.abc &&
abcm2ps us.abc -O= &&
evince us.ps
