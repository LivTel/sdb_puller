#!/usr/bin/env bash

##################################################################################
# Script to                                                                      #
#     - Use the TTL Std (Sdb Test Dump) tool to obtain sdb date for each hour    #
#     - Parse the data into INFLUX db compatable ingestion files                 #
#     - Ingest into INFLUX db Database                                           #
#                                                                                #
#                                                                                #
#    DMA 04/04/2019                                                              #
##################################################################################


#############################
# ENV Vars                  #
#############################

BASEPATH=/ttl/sw
UTILPATH=$BASEPATH/util/
CONFIGFILE=migrate.cfg



main()
{

cp $BASEPATH/etc/$CONFIGFILE ./

   shopt -s nullglob # Stops file being set to "*.gz" if no files exist in current directory
   for file in *.gz
   do
      getDateTime "$file"
      modifyConfig "$file"
      runStd
      convert2csv "$date.dat" "$date.csv"
      influx_parse "$date.csv" "$date.flx"
   done

   echo utilpath is $UTILPATH
   #cleanup
}

#############################
#   Functions               #
#############################

cleanup()
{
   rm *.cfg
   rm *.dat
   rm *.csv
}


convert2csv()
{
   # Change tab delimited output to comma delimited
   echo Converting $1 file into $2
   cat $1 | sed 's/\t/,/g' > $2
}

getDateTime()
{
   date=${1:0:8}
   year=${1:0:2}
   month=${1:2:2}
   day=${1:4:2}
   hour=${1:6:2}

   #debug
   #echo year is $year
   #echo month is $month
   #echo day is $day
   #echo hour is $hour
}

influx_parse()
{
   $BASEPATH/bin/influx_parse.py $1 $2 TABLE
}

modifyConfig()
{
   # Change the config file based on the time of the file
   cat migrate.cfg | sed "s/M_DATE/$year\/$month\/$day/g" | sed "s/M_HOUR/$hour/g" | sed "s/M_PLUS_HOUR/$((hour+1))/g" | sed "s/M_OUTPUT/$date/g" > $date.cfg
}

runStd()
{
   $UTILPATH/Std -conf $date.cfg -path ./
}



# Run main function
main