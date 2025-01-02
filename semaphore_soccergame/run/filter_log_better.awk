BEGIN {
     FS = " ";
     f=1
     #players
     for(i=0;i<numplayers;i++) {
        FieldSize[f++] = 4;
     }
     #goalies
     FieldSize[f++]=5;
     for(i=0;i<numgoalies;i++) {
        FieldSize[f++] = 4;
     }
     #referee
     FieldSize[f++]=5;
}
/.*/ {
    if(NF==totalentities) {
#        print  "NOTFILTE " $0
        for(i=1; i<=totalentities; i++) {
               if($i==prev[i]) {
                 printf("%*s ",FieldSize[i],".")
               }
               else printf ("%*s ",FieldSize[i],$i) 
               prev[i] = $i
        
        }
        printf("\n")
    }
    else print $0
}

END{
}
