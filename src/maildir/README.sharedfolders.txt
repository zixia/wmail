
                                Shared folders

   This   document  describes  how  shared  folders  are  implemented  by
   Courier-IMAP,  and SqWebMail, within the framework of the Maildir mail
   store format. Having the ability to implement shared folders was badly
   needed for some time. And finally, there's a way to do it!

Terminology

     * Sharable  Maildir  -  a  maildir that contains folders that can be
       shared.
     * Sharable  folder  - one or more folders in a sharable Maildir that
       can be shared.

Technical Overview

     * They are a somewhat logical extension of the base Maildir format.
     * Older  versions  of  Courier-IMAP  and  SqWebMail  will completely
       ignore shared folders.
     * Messages  in  shared  folders  do not count towards any individual
       maildir quotas (see README.maildirquota.html).
     * Shared   folders  are  implemented  as  additional,  Maildir-based
       folders.  The  messages  will be soft links to the messages in the
       sharable  folder. Soft links will be used instead of hard links in
       order  to  be able to have a folder containing large messages, and
       then  be  able  to reclaim space immediately when the messages are
       purged,  instead  of  waiting  for  everyone to sync up and delete
       their  hard  link.  The flip side is that you can expect the usual
       sorts  of  errors  and  increased confusion if someone attempts to
       read  a  message  that  has  just  been  removed, but their client
       (Courier-IMAP   or   SqWebMail)  doesn't  know  that  yet.  That's
       unavoidable.  You'll probably have to set some kind of a policy in
       order to keep the expected insanity to a minimum.
     * Sharable   folders  are  subscribed  to  by  creating  a  separate
       maildir-type  directory within the main Maildir. It's created in a
       separate subdirectory that is ignored by other Maildir clients.
     * The  new directory will contains soft links to the messages in the
       sharable   folder.   "Synchronization"   code   will  be  used  to
       synchronize  the  contents of the sharable folder, and the copy of
       it in the regular Maildir. Once links to the messages are created,
       they  can  be manipulated like regular messages in a Maildir. This
       procedure  will  be  transparently  performed  by  Courier-IMAP or
       SqWebMail behind the scenes.
     * Access  to  shared  folders  is  controlled by a combination of an
       explicit  configuration,  plus  regular filesystem permissions. If
       account  owners  have  shell access to the server, they can create
       shared  folders,  and link their mailbox to other accounts' shared
       folders.   Then,  the  actual  access  is  controlled  by  regular
       filesystem  permissions.  The  owner of the shared folder will use
       the regular filesystem permission to give read and/or write access
       to either everyone else, or everyone else who uses the same system
       group ID. Read access allows people to read messages from a shared
       folder.  Write  access allows people to add and delete messages in
       the shared. The owner of the shared folder can remove any message,
       everyone else will be able to remove only their own messages.

Functional Overview

   Generally   speaking,   shared   folders   are   configured   using  a
   feature-enhanced maildirmake command as follows:
     * make  install  will install a maildirmake command that has a bunch
       of  funny  options.  The modified maildirmake command is installed
       into Courier-IMAP's or SqWebMail's installation directory.
     * Somebody,  maybe  you,  will use some of these options to create a
       maildir  with  modified  permissions.  The  same somebody will run
       maildirmake  again, with a few other options, to create folders in
       that  maildir,  also  with modified permissions. Those permissions
       allows global read (and optionally write) access to those folders.
     * Do  NOT  use  this  maildir  as the primary mailbox, INBOX, for an
       account. Instead, you must create this maildir separately, perhaps
       as  $HOME/Maildir-shared,  then  set it up as one of your sharable
       maildirs  (see  below),  and access it in shared mode. Because you
       own it, you have unlimited read/write access to it. The previously
       mentioned  options  will  select whether or not access permissions
       are given to everyone else, and they do not apply to you.
     * Everyone  else  will run maildirmake with even more funny options.
       Those  options  install a link from their own maildir to a maildir
       that  contains sharable folders. A given maildir may contain links
       to  more  than  one  sharable  maildir.  As  long  as their system
       user/group  permissions  allow  them  to access messages, they can
       SUBSCRIBE  via  their  IMAP  client  to  the  folder,  or  use the
       SUBSCRIBE function in SqWebmail.
     * If  people do not have shell access, the system administrator will
       have  to  run  maildirmake on their behalf, in order to create the
       links to the sharable maildirs.
     * People  with  write  access can add messages to a sharable folder.
       Messages  from  a sharable folder can be removed only by the owner
       of the shared folder, or by the owner of each message.
     * This   sharable   maildir   implementation   cannot   use  maildir
       soft-quotas.  There cannot be a soft-quota installed in a sharable
       maildir.  If  you  need  quota  support,  you  will  have  to  use
       filesystem-based   quotas.  There  are  some  sticky  issues  with
       updating quotas on a sharable maildir.

   Also,  note that anyone, not just the superuser, can create a sharable
   maildir in their account, and invite anyone else to access it, as long
   as their system user/group permissions allow them access.

   To summarize:
     * Use  the  -S  option  to  maildirmake to create a maildir that can
       contain sharable folders.
     * Use  the  -s  and  -f  options  to  maildirmake to create sharable
       folders.  The  same sharable maildir may contain multiple sharable
       folders  with  different access permissions, as selected by the -s
       option.
     * Use the --add option to install a link from a regular maildir to a
       sharable  maildir.  Afterwards,  IMAP  clients  will  be  able  to
       subscribe to folders in the sharable maildir.
     * Use the --del option to remove a link.

   For more information on these options, see the maildirmake manual page
   that will be installed.

More on additional options for the maildirmake command

   The  '-S'  option to maildirmake to create a maildir that will contain
   shared  folders.  The -S option gives group and world read permissions
   on  the maildir itself (where group/world permissions are normally not
   set  for  a regular maildir). This allows access to any folders in the
   shared  maildir,  and  that's  why  you  should  not  use this Maildir
   directly as your primary mailbox.

   The  "new"  and  "cur"  subdirectories  will  not  be  used or shared,
   although  they  will still be created. Both SqWebMail and Courier-IMAP
   create  their  auxiliary  files in the main Maildir with the group and
   world  permissions  turned  off,  so  this maildir can, theoretically,
   still be used as the primary INBOX, but I don't recommend that.

   The  -S  option  is  not limited to the system administrator. In fact,
   anyone  can  use  the  -S  option, to create shared maildirs that they
   maintain.

   Shared  folders are created like any other folder, using the -f option
   to  maildirmake.  However,  that normally creates a folder that is not
   sharable,  because  it  will  not have any group or world permissions.
   Therefore,  maildirmake  will  take  the following options to create a
   sharable folder:
     * -s  read  will  create  a "moderated" folder. The folder will have
       group  and  world  read permissions, letting anyone read messages,
       but only the owner of the sharable Maildir can add messages to the
       sharable folder.
     * -s write will create an "unmoderated" folder. The folder will have
       group  and  world  read/write permissions, letting anyone read and
       add  messages  to  the folder. The folder will be created with the
       sticky  bit  set,  like the /tmp directory, preventing people from
       removing someone else's messages.
     * -s  read,group/-s  write,group append a comma and the word "group"
       to  modify the semantics of moderated and unmoderated folders only
       on  the  group  level,  not  the  group and world level, as far as
       permissions  go. This restricts sharing the folder only to members
       of the same system group as the owner of the sharable maildir.

   It's  worth noting that it is perfectly permissible for folders in the
   same sharable maildir to have different access levels.

   Also,   this   is   driven  entirely  by  filesystem  permissions,  so
   theoretically  it's  possible  to  create  a  folder  that  has  write
   permissions  for  the  group,  and read permissions for everyone else.
   However,  I'm  too  lazy  to  actually  do  it.  Feel  free  to  patch
   maildirmake to add this functionality, then send me the patch.

Accessing shared folders

   The rest of the document consists of technical implementation notes.

   Accessing  a collection of shared folders is implemented by a new file
   that  is installed in the primary maildir (usually $HOME/Maildir), and
   a new subdirectory hierarchy underneath the primary maildir, which are
   hereby declared.

  shared-maildirs

   This  file  must  be  created  by  the administrator or by the maildir
   owner,  manually. This file lists all available sharable maildirs that
   this  maildir  can  access  in  shared mode (confused yet?). This file
   contains one or more lines of the following format:

   name /path/to/shared/maildir

   "name"  is an arbitrary name that's given to this collection of shared
   folders. The name may not contain slashes, periods, or spaces. This is
   followed  by a pathname to the maildir containing shared folders. Note
   that  this  is  NOT  the  sharable folder itself, but the maildir that
   contains one or more sharable folders. The maildir client will be able
   to selectively subscribe to any sharable folder in that maildir.

  shared-folders

   This  subdirectory  forms  the root of all the shared folders that are
   subscribed  to.  Let's  say  that  shared-maildirs  has  the following
   contents:

   firmwide /home/bigcheese/Maildir-shared
   tech /home/gearhead/Maildir-shared

   Subscribing  to  folders  'golf'  and  'boat'  in  bigcheese's  shared
   Maildir,  and to the folder 'maintenance' in gearhead's shared Maildir
   involves       creating       the       following      subdirectories:
   shared-folders/firmwide/.golf,    shared-folders/firmwide/.boat,   and
   shared-folders/tech/.maintenance.

  shared

   This    is    a    soft   link   that's   automatically   created   in
   shared-folder/name.  It  is  a  soft  link that points to the sharable
   maildir, which contains the sharable folders.

Subscribing to a shared folder

     * Read shared-maildirs.
     * Read  the  shared-folders hierarchy to determine which folders are
       already subscribed to.
     * Read  the  folders  in  each maildir listed in shared-folders, and
       ignore  the  ones  that  are  already  subscribed to. Make sure to
       stat() each folder to make sure that you can read it.
     * If   necessary,   create   shared-folders/name,   and  create  the
       shared-folders/name/shared soft link.
     * Create  shared-folders/name/.foldername,  then create the standard
       tmp,  new, and cur subdirectories. All the directories are created
       without any group or world permissions.

Unsubscribing

     * Delete everything in shared-folders/name/.foldername.
     * If  this  is there are no more subscribed folders in this sharable
       maildir, delete shared-folders/name for good measure as well.

Opening a shared folder

   The  process of "opening" a shared folder involves basically "syncing"
   the  contents  of  shared-folder/name/.foldername with the contents of
   the sharable folder in the original sharable maildir.
     * Do the usual processing on the tmp subdirectory.
     * Read the contents of shared-folder/name/shared/.foldername/new. If
       you  find  anything  in  there, rename it to .../cur, ignoring any
       errors from the rename. The sharable maildir owner can arrange for
       mail to be delivered directly to this folder, and the first one to
       open it will put the message into cur.
     * Read  the  contents  of shared-folder/name/shared/.foldername/cur.
       Call this directory "A", for short.
     * Read  the  contents  of shared-folder/name/shared/.foldername/cur.
       Call this directory "B", for short.
     * stat() A and B to see if they live on different devices.
     * Remove  all  messages  in  B  that  do not exist in A. You want to
       compare the filenames up to the : character.
     * Create  soft  links from messages that exist in A and do not exist
       in  B.  The name in B should not have any flags after the :, so it
       shows up as a new message.
     * You  can now do whatever you normally do with a maildir. Note that
       new  is  completely ignored, though. Moving messages IN and OUT of
       the shared folder involves copying the message as if it were a new
       message.  Copying the message INTO the shared folder means copying
       the   message   into  the  underlying  sharable  folder's  tmp/cur
       directory, and it will show up after the next sync.
