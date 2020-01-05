#!/bin/sh
#!

#
menuexit ()
{ 
  echo "IPF aborted"
  exit
}

menu ()
{
  select1=-1
  select2=-1
  while [ $select1 -lt 0 ]; do
    echo "**************************************************************"
    echo " Interactive Powerflow (IPF) Programs Main Menu"
    echo " 0. (or <RETURN>) -- Done! Quit."
    echo " 1. BPF       -- IPF batch powerflow "
    echo " 2. IPFPLOT   -- Plot coordinate file (or master) and base file"
    echo " 3. PLOTGUI   -- (Not available on Windows NT)"
    echo " 4. GUI       -- XWindows interface with IPFSRV"
    echo " 5. IPFNET    -- Network data extract"
    echo " 6. IPFCUT    -- Cut PF system"
    echo " 7. IPS2IPF   -- (Not available - obsolete!)"
    echo " 8. IPF_UTILS -- IPF_REPORTS version of PSAP"
    echo " 9. IPFBAT    -- Batch version of IPFSRV"
    echo "10. FINDOUT   -- Fast Outage post processor (new)"
    echo "11. PVCURVE   -- CFLOW program to generate PV data"
    echo "12. QVCURVE   -- CFLOW program to generate QV data"
    echo "13. POST_PVCURVE -- CFLOW program to post process PV data"
    echo "14. LINEFLOW  -- CFLOW program to print selected lines"
    echo ""
    echo "Enter selection: "
    read select1
    if [ $select1 -eq 0 ];then
      exit
    elif [ $select1 -gt 14 ]; then
      echo "Invalid selection"
      select1=-1
    elif [ $select1 -eq 1 ]; then
      select2=-1
      while [ $select2 -lt 0 ]; do
        echo "Batch PF menu"
        echo ""
        echo "0. Done! Quit."
        echo "1. Batch BPF only"
        echo "2. Batch BPF and IPFPLOT"
        echo "3. Interactive IPF"
        echo ""
        echo "Enter selection: "
        read select2 
        if [ $select2 -gt 3 ]; then
          echo "Invalid selection"
          select2=-1
        fi
      done
    fi
  done
}

get_new_pf_base()
{
# echo "new_pf_base: file: $1"
  {   
    while read line; do
      case $line in

      /*NEW_BASE*|/*new_base* )
#       echo "PF command [$line]"
        IFS=" ,="
        count=0
        state=0
        for token in $line; do
#         echo " state [$state] token[$count] = [$token]"
          count=count+1
          case $state in
            0 )
              if [ "$token" = "FILE" ] || [ "$token" = "file" ]; then
                state=1
              fi ;;

            1 )
              if [ "$token" != "" ]; then
                new_base_file=$token
#               echo "New base file = [$new_base_file]"
                break
              fi ;;
          esac
        done ;;

      * )
#       echo "PF record  [$line]" ;;
      esac
    done
  } < $1

}

process_menu_selection()
{
  status=0
  case $select1 in
#
#      Batch IPF/IPFPLOT
#
    1) finished=0
       while [ $finished -eq 0 ]; do
         echo "Enter pfc file name:"
         read pfc_file
         if [ -z $pfc_file ]; then
           echo "IPF aborted by Null selection"
           exit
         elif [ ! -e $pfc_file ]; then
           echo "PFC file $pfc_file does not exist"
         else
           finished=1
         fi
       done
#      echo "pfc_file: $pfc_file"
       if [ $select2 -lt 3 ]; then
         pfc_batch_file="${pfc_file}.batch"
         pfc_log_file="${pfc_file}.log"
#        echo "pfc_batch_file: $pfc_batch_file"
         {
           echo "#!/bin/sh"
           echo "#"
           echo "# **************************************************************"
           echo "# Launching bpf/ipfplot $pfc_file"
           echo "# **************************************************************"
           echo "#" 
           echo "date"
           echo "/bin/bpf $pfc_file > $pfc_log_file"
         } > $pfc_batch_file
         if [ $select2 -eq 2 ]; then
           new_base_file=""
           get_new_pf_base $pfc_file
           echo "new_base_file: $new_base_file"
           if [ -z $new_base_file ]; then
             echo "No NEW_BASE file specified in pfc file $pfc_file"
             exit
           fi
           finished=0
           while [ $finished -eq 0 ]; do
             echo "Enter Coordinate file name:"
             read cor_file
             if [ -z $cor_file ]; then
               echo "IPF aborted by Null selection"
               exit
             elif [ ! -e $cor_file ]; then
               echo "Coordinate file $cor_file does not exist"
             else
               finished=1
             fi
           done
           if [ ! -e $IPFDIRS/pfmaster.post ]; then
             echo "IPFPLOT aborted!"
             echo "pfmaster.post file is not installed in directory IPFDIRS!"
             exit
           fi
           {
             echo "#"
             echo "# **************************************************************"
             echo "# Launching ipfplot $cor_file $new_base_file"
             echo "# **************************************************************"
             echo "#" 
             echo "date"
             echo "/bin/ipfplot $cor_file $new_base_file >> $pfc_log_file"
           } >> $pfc_batch_file
         fi
         echo "date" >> $pfc_batch_file
         chmod a+x $pfc_batch_file
         echo "**************************************************************"
         echo "Launching bpf batch file $pfc_batch_file"
         echo "              log file   $pfc_log_file"
         echo "**************************************************************"
         if [ $select2 -gt 1 ]; then
           new_ps_file=${cor_file%.*}_${new_base_file%.*}.ps
           echo "**************************************************************"
           echo "IPFPLOT diagram in file ${new_ps_file}"
           echo "**************************************************************"
         fi
         ./$pfc_batch_file &
#
#        Interactive IPF
#
       else
         bpf $pfc_file
       fi ;;
#
#      Batch IPFPLOT
#
    2) finished1=0
       finished2=0
       old_cor_file=""
       old_base_file=""
       while [ $finished1 -eq 0 ]; do
         finished2=0
         while [ $finished2 -eq 0 ]; do
           echo "Enter Coordinate file name:"
           read cor_file
           if [ -z $cor_file ]; then
             if [ -z $old_cor_file ]; then
               echo "IPF aborted by Null selection"
               exit
             else
               cor_file=$old_cor_file
             fi
           fi
           if [ ! -e $cor_file ] && [ "$cor_file" != "$old_cor_file" ]; then
             if [ ! -e $BASECASE_DIR$cor_file ]; then
               $echo "Coordinate file $cor_file does not exist"
             else
               finished2=1
             fi
           else
             finished2=1
           fi
         done
         if [ ! -e $IPFDIRS/pfmaster.post ]; then
           echo "IPFPLOT aborted!"
           echo "pfmaster.post file is not installed in directory IPFDIRS!"
           exit
         fi
         finished2=0
         while [ $finished2 -eq 0 ]; do
           echo "Enter Basecase file name:"
           read base_file
           if [ -z $base_file ]; then
             if [ -z $old_base_file ]; then
               echo "IPF aborted by Null selection"
               exit
             else
               base_file=$old_base_file
             fi
           fi
           if [ ! -e $base_file ] && [ "$base_file" != "$old_base_file" ]; then
             if [ ! -e $BASECASE_DIR$base_file ]; then
               $echo "Basecase file $base_file does not exist"
             else
               finished2=1
             fi
           else
             finished2=1
           fi
         done
	 if [ "$cor_file" = "$old_cor_file" ] && [ "$base_file" = "$old_base_file" ]; then
	   finished1=1
	 else
	   old_cor_file=$cor_file
	   old_base_file=$base_file
           pfc_batch_file="${cor_file}.batch"
           {
             echo "#"
             echo "# **************************************************************"
             echo "# Launching ipfplot $cor_file $base_file"
             echo "# **************************************************************"
             echo "#" 
             echo "date"
             echo "ipfplot $cor_file $base_file"
             echo "date" 
           } >> $pfc_batch_file
           chmod a+x $pfc_batch_file
           echo "**************************************************************"
           echo "Launching ipfplot batch file $pfc_batch_file"
           echo "**************************************************************"
           pfc_log_file=${pfc_batch_file#*.}.log
           ./$pfc_batch_file >> $pfc_log_file 
           new_ps_file=${cor_file%.*}_${base_file%.*}.ps
           echo "**************************************************************"
           echo "IPFPLOT diagram in file ${new_ps_file}"
           echo "**************************************************************"
	 fi
       done
       ;;
#
#      GUI PLOT
#
    3) echo "GUI PLOT is not available" ;;
#
#      GUI 
#
    4) echo "GUI is not available" ;;
#
#      IPFNET
#
    5) netdat ;;
#
#      IPFCUT
#
    6) cutting ;;
#
#      IPS2IPF
#
    7) echo "IPS->IPF conversion utility program is not available" ;;
#
#      PF_UTILITIES
#
    8) ipf_reports ;;
#
#      IPFBAT
#
    9) finished=0
       while [ $finished -eq 0 ]; do
         echo "Enter pcl file name:"
         read pfc_file
         if [ -z $pfc_file ]; then
           echo "IPF aborted by Null selection"
           exit
         elif [ ! -e $pfc_file ]; then
           echo "PCL file $pfc_file does not exist"
         else
           finished=1
         fi
       done
#      echo "pfc_file: $pfc_file"
       pfc_batch_file="${pfc_file}.batch"
#      echo "pfc_batch_file: $pfc_batch_file"
       {
         echo "#!/bin/sh"
         echo "#"
         echo "# **************************************************************"
         echo "# Launching ipfbat $pfc_file"
         echo "# **************************************************************"
         echo "#" 
         echo "date"
         echo "ipfbat $pfc_file"
       } > $pfc_batch_file
       ./$pfc_batch_file ;;
#
#      FINDOUT
#
   10) findout ;;
#
#      PVCURVE
#
   11) pvcurve ;;
#
#      QVCURVE
#
   12) qvcurve ;;
#
#      POST_PVCURVE
#
   13) post_pvcurve ;;
#
#      LINEFLOW
#
   14) lineflow ;;

    *) echo "Invalid response" ;;

   esac
}

trap menuexit INT TERM

select1=-1
select2=-1
pfc_file=""
cor_file="" 
pfc_batch_file=""
new_base_file=""

menu
process_menu_selection
exit


