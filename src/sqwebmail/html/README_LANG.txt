
                        SqWebMail Language translations

   Starting  with  version 0.23, SqWebMail includes configuration scripts
   in  the  html  subdirectory  (the one where you found this file) which
   make  it  possible  to  be  able  to  add translated HTML templates to
   SqWebMail  very  easily.  You  will be able to create and maintain the
   translated HTML templates all by yourself.

   Before  you  set  up  a translated HTML template set, you will need to
   have the following information:
     * ISO  code/locality for your translation. For example, I distribute
       SqWebMail  with the HTML templates for en-us - English-US. If, for
       example,   you   wanted   to  create  an  HTML  template  set  for
       English-British,  your  ISO  code/locality  would be en-gb. If you
       wanted  to  create  an  HTML  template  set  for  French, your ISO
       code/locality would be fr-fr, and so on.
     * Locale  code. This is the name of the i18n locale that's installed
       on  your  system.  The  locale  primarily determines how dates are
       displayed.  It  is  usually very similar to the ISO code/locality.
       For  example,  the  i18n  locale  for  English-US  is  en_US,  for
       English-British  it's  en_GB,  for French it's fr_FR. Try checking
       for   the   LOCALE  environment  variable,  the  manual  page  for
       locale(7), or the contents of /usr/share/locale directory.
     * The  MIME  character  set.  This  specifies  the  character set of
       messages  in  this  language.  For  most european languages, it is
       iso-8859-1.
     * The  name of the ispell dictionary for your language. For example,
       for  English-US,  the  corresponding  ispell  dictionary is called
       "american", for German, it's "german".

Extract sqwebmail, and run configure

   First,  you  want  to take the standard sqwebmail-0.23.tar.gz tarball,
   and extract it, then run the configure script with your usual options:

   $ tar xzvf sqwebmail-0.23.tar.gz
   $ cd sqwebmail-0.23
   $ ./configure [ your usual options go here ]

   Change to the html subdirectory.

   $ cd html

   The   standard  sqwebmail-@VERSION.tar.gz  includes  the  subdirectory
   html/en-us. Let's say you want to translate it into French.

   The ISO code/locality is fr-fr.

   The locale code is fr_FR.

   The ispell dictionary name is "french".

   The   first  thing  that  needs  to  be  done  is  to  create  another
   subdirectory  in  html.  The  name of the subdirectory must be the ISO
   code/locality for the language.

   You  don't  have  to  do  it  by  hand.  There's  a  special target in
   html/Makefile that will do it for you!

   $ make clone from=en-us to=fr-fr

   This  script takes the Makefile, the configure script, and other files
   from   the   html/en-us   subdirectory,  and  creates  the  html/fr-fr
   subdirectory  which  will  temporary  contain  the mirror image of the
   html/en-us subdirectory.

Setting LANGUAGE_PREF

   You  now  have  to  make  minor  changes  to  some  files in the fr-fr
   subdirectory. fr-fr/LANGUAGE will be automatically created by the make
   clone  script.  However,  you  must  initialize  the  contents  of the
   fr-fr/LANGUAGE_PREF file:

   $ echo fr50 fr-fr >fr-fr/LANGUAGE_PREF

   The  LANGUAGE_PREF  file must contain exactly one line with two words.
   The  second  word must always be the ISO code/locality. The first word
   is used to sort all the installed HTML templates alphabetically by ISO
   code/locality,  and it's used to selecte the preferred locality within
   the same ISO code.

   For  example,  the  contents  of  en-us/LANGUAGE_PREF is "en50 en-us".
   Let's say you want to have both en-us and en-gb HTML templates. If you
   want  clients  requesting the "en" HTML to receive the en-gb HTML, you
   will have to set en-gb/LANGUAGE_PREF to something like "en40 en-gb":

   $ echo en40 en-gb >en-gb/LANGUAGE_PREF

   If  you  want  clients  requesting  the "en" HTML to receive the en-us
   HTML, you will have to do something like this:

   $ echo en60 en-gb >en-gb/LANGUAGE_PREF

   The first word in all the LANGUAGE_PREF files is used to sort the list
   of  all  the  available HTML templates, and the sorted list is used to
   select the preferred locality within the same ISO code.

Setting LOCALE

   Let's  resume  the  example with the French translation. The next file
   that needs to be changed is LOCALE. This file contains the locale code
   for the language. For French, the locale code is fr_FR:

   $ echo fr_FR >fr-fr/LOCALE

Setting CHARSET

   The CHARSET file sets the MIME character set of outgoing messages. For
   US and most european languages, this value should be iso-8859-1

   $ echo iso-8859-1 >fr-fr/CHARSET

Setting ISPELLDICT

   Finally,  you  need  to  specify  which dictionary ispell will use for
   spell checking. For French, the dictionary is simply named "french":

   $ echo french >fr-fr/ISPELLDICT

   If  you do not have an ispell dictionary for this language, initialize
   ISPELLDICT with the name of the default dictionary.

Rerunning the configure script

   You  now  need  to  rerun  the  configure script in the main sqwebmail
   directory:

   $ cd ..
   $ ./configure [ options ]

   This  reruns  the  configure  script  in the sqwebmail-0.23 directory,
   which will now pick up your new html/fr-fr subdirectory, which will be
   configured in.

Creating the actual translations

   Now,  you  can edit all the .html files in html/fr-fr, and replace all
   English text with its French equivalent.

   If  you  now  do  a  make,  followed by make install, the installation
   script  will  install both the original en-us HTML templates and fr-fr
   HTML templates together.

   If you now run make dist:

   $ make dist

   This  will create a new sqwebmail-0.23.tar.gz tarball, containing both
   sets of HTML templates, which you can now distribute!

Updating translations for new versions of SqWebMail

   Well, what should you do when a new version of SqWebMail is available?

   First,  you need to determine whether or not there were any changes to
   the  contents  of  the html/en-us subdirectory. Then, you will have to
   apply  the  same  changes  to  the  translated  contents  of  the  old
   html/fr-fr subdirectory.

   Then,  repeat the previous procedure for the new version of SqWebMail,
   but  before running the configure script for the second time, copy all
   the  .html  files  from  the  previous  version of SqWebMail (plus any
   changes  or  new  files)  into  the html/fr-fr subdirectory of the new
   version.
