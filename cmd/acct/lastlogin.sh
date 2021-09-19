#	"lastlogin - keep record of date each person last logged in"
#	"bug - the date shown is usually 1 more than it should be "
#	"       because lastlogin is run at 4am and checks the last"
#	"       24 hrs worth of process accounting info (in pacct)"
PATH=/usr/lib/acct:/bin:/usr/bin:/etc
cd /usr/adm/acct
if test ! -r sum/loginlog; then
	nulladm sum/loginlog
fi
#	"cleanup loginlog - delete entries of those no longer in"
#	"/etc/passwd and add an entry for those recently added"
#	"line 1 - get file of current logins in same form as loginlog"
#	"line 2 - merge the 2 files; use uniq to delete common"
#	"lines resulting in those lines which need to be"
#	"deleted or added from loginlog"
#	"line 3 - result of sort will be a file with 2 copies"
#	"of lines to delete and 1 copy of lines that are "
#	"valid; use uniq to remove duplicate lines"
cat /etc/passwd | sed "s/\([^:]*\).*/00-00-00  \1/" |\
sort +1 - sum/loginlog | uniq -u +10 |\
sort +1 - sum/loginlog |uniq -u > sum/tmploginlog
cp sum/tmploginlog sum/loginlog
#	"update loginlog"
d="`date +%y-%m-%d`"
day=`date +%m%d`
#	"lines 1 and 2 - remove everything from the total"
#	"acctng records with connect info except login"
#	"name and adds the date"
#	"line 3 - sorts in reverse order by login name; gets"
#	"1st occurrence of each login name and resorts by date"
acctmerg -a < nite/ctacct.$day | \
 sed -e "s/^[^ 	]*[ 	]\([^ 	]*\)[ 	].*/$d  \1/" | \
 sort -r +1 - sum/loginlog | uniq +10 | sort >sum/tmploginlog
cp sum/tmploginlog sum/loginlog
rm -f sum/tmploginlog
