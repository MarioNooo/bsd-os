#
# /usr/share/sendmail/cf/bsdi.mc
#
# Do not edit /etc/mail/sendmail.cf directly unless you cannot do what you
# want in the master config file (/usr/share/sendmail/cf/bsdi.mc).
# To create /etc/mail/sendmail.cf from the master:
#     cd /usr/share/sendmail/cf
#     mv /etc/mail/sendmail.cf /etc/mail/sendmail.cf.save
#     m4 < bsdi.mc > /etc/mail/sendmail.cf
#
# Then kill and restart sendmail:
#     sh -c 'set `cat /var/run/sendmail.pid`; kill $1; shift; eval "$@"'
#
# See /usr/share/sendmail/README for help in building a configuration file.
#
# You might also find checksendmail(8) helpful in debugging .cf files:
#     checksendmail -r /usr/contrib/lib/address.resolve
#
include(`../m4/cf.m4')
VERSIONID(`@(#)BSDI $Id: bsdi.mc,v 1.7 2000/01/21 16:41:23 polk Exp $')dnl
OSTYPE(`bsdi')dnl
DOMAIN(`generic')dnl

dnl # NOTE: `dnl' is the m4 command for delete-to-newline; these are
dnl # used to prevent those lines from appearing in the sendmail.cf.
dnl #
dnl # UUCP-only sites should configure FEATURE(`nodns') and SMART_HOST.
dnl # The uucp-dom mailer requires MAILER(smtp).  For more info, see
dnl # `UUCP Config' at the end of this file.

dnl # If you are not running DNS you need to add:
dnl #   echo 'hosts     files' >> /etc/service.switch

dnl # Use FEATURE(`nocanonify') to skip address canonification via $[ ... $].
dnl # This would generally only be used by sites that only act as mail gateways
dnl # or which have user agents that do full canonification themselves.
dnl # You may also want to use:
dnl #     define(`confBIND_OPTS',`-DNSRCH -DEFNAMES')
dnl # to turn off the usual resolver options that do a similar thing.
dnl # Examples:
dnl     FEATURE(`nocanonify')
dnl     define(`confBIND_OPTS',`-DNSRCH -DEFNAMES')

dnl # If /bin/hostname is not set to the FQDN (Full Qualified Domain Name;
dnl # for example, foo.bar.com) *and* you are not running a nameserver
dnl # (that is, you do not have an /etc/resolv.conf and are not running
dnl # named) *and* the canonical name for your machine in /etc/hosts
dnl # (the canonical name is the first name listed for a given IP Address)
dnl # is not the FQDN version then define NEED_DOMAIN and specify your
dnl # domain using `DD' (for example, if your hostname is `foo.bar.com'
dnl # then use DDbar.com).  If in doubt, just define it anyway; doesn't hurt.
dnl # Examples:
dnl     define(`NEED_DOMAIN', `1')
dnl     DDyour.site.domain

dnl # Define SMART_HOST if you want all outgoing mail to go to a central
dnl # site.  SMART_HOST applies to names qualified with non-local names.
dnl # Example:
dnl     define(`SMART_HOST', `smtp:firewall.bar.com')

dnl # Define MAIL_HUB if you want all incoming mail sent to a
dnl # centralized hub, as for a shared /var/spool/mail scheme.
dnl # MAIL_HUB applies to names qualified with the name of the
dnl # local host (e.g., "eric@foo.bar.com").
dnl # Example:
dnl     define(`MAIL_HUB', `smtp:mailhub.bar.com')

dnl # LOCAL_RELAY is a site that will handle unqualified names, this is
dnl # basically for site/company/department wide alias forwarding.  By
dnl # default mail is delivered on the local host.
dnl # Example:
dnl     define(`LOCAL_RELAY', `smtp:mailgate.bar.com')

dnl # Relay hosts for fake domains: .UUCP .BITNET .CSNET
dnl # Examples:
dnl     define(`UUCP_RELAY', `mailer:your_relay_host')
dnl     define(`BITNET_RELAY', `mailer:your_relay_host')
dnl     define(`CSNET_RELAY', `mailer:your_relay_host')

dnl # Define `MASQUERADE_AS' is used to hide behind a gateway.
dnl # add any accounts you wish to be exposed (i.e., not hidden) to the
dnl # `EXPOSED_USER' list.
dnl # Example:
dnl     MASQUERADE_AS(`some.other.host')

dnl # If masquerading, EXPOSED_USER defines the list of accounts
dnl # that retain the local hostname in their address.
dnl # Example:
dnl     EXPOSED_USER(`postmaster hostmaster webmaster')

dnl # If masquerading is enabled (using MASQUERADE_AS above) then
dnl # FEATURE(allmasquerade) will cause recipient addresses to
dnl # masquerade as being from the masquerade host instead of
dnl # getting the local hostname.  Although this may be right for
dnl # ordinary users, it breaks local aliases that aren't exposed
dnl # using EXPOSED_USER.
dnl # Example:
dnl     FEATURE(allmasquerade)

dnl # Enable anti-SPAM features
FEATURE(relay_entire_domain)
FEATURE(relay_based_on_MX)
FEATURE(access_db)
FEATURE(blacklist_recipients)
dnl # If you uncomment this line, you will reject mail from the 
dnl # "Realtime Blackhole List" run by the MAPS project (see 
dnl # http://maps.vix.com)
dnl	FEATURE(rbl)

dnl # Turn on mailertable and virtusertable for class T
FEATURE(mailertable)
FEATURE(virtusertable)
VIRTUSER_DOMAIN_FILE(`/etc/mail/virtual-host-names')

dnl # Include any required mailers
MAILER(local)dnl
MAILER(smtp)dnl
MAILER(uucp)dnl

LOCAL_CONFIG
# Localizations for the configuration section (e.g., defining local 
# classes) go here...

LOCAL_RULE_0
# `LOCAL_RULE_0' can be used to introduce alternate delivery rules.
# For example, let's say you accept mail via an MX record for widgets.com
# (don't forget to add widgets.com to your Cw list, as above).
#
# If wigets.com only has an AOL address (widgetsinc) then you could use:
# R$+ <@ widgets.com.>  $#smtp $@aol.com. $:widgetsinc<@aol.com.>
#
# Or, if widgets.com was connected to you via UUCP as the UUCP host
# widgets you might have:
# R$+ <@ widgets.com.>   $#uucp $@widgets $:$1<@widgets.com.>

dnl ###
dnl ### UUCP Config
dnl ###

dnl # `SITECONFIG(site_config_file, name_of_site, connection)'
dnl # site_config_file the name of a file in the cf/siteconfig
dnl #                  directory (less the `.m4')
dnl # name_of_site     the actual name of your UUCP site
dnl # connection       one of U, W, X, or Y; where U means the sites listed
dnl #                  in the config file are connected locally;  W, X, and Y
dnl #                  build remote UUCP hub classes ($=W, etc).
dnl # You will need to create the specific site_config_file in
dnl #     /usr/share/sendmail/siteconfig/site_config_file.m4
dnl # The site_config_file contains a list of directly connected UUCP hosts,
dnl # e.g., if you only connect to UUCP site gargoyle then you could just:
dnl #   echo 'SITE(gargoyle)' > /usr/share/sendmail/siteconfig/uucp.foobar.m4
dnl # Example:
dnl     SITECONFIG(`uucp.foobar', `foobar', U)

dnl # If you are on a local SMTP-based net that connects to the outside
dnl # world via UUCP, you can use LOCAL_NET_CONFIG to add appropriate rules.
dnl # For example:
dnl #   define(`SMART_HOST', suucp:uunet)
dnl #   LOCAL_NET_CONFIG
dnl #   R$* < @ $* .$m. > $*    $#smtp $@ $2.$m. $: $1 < @ $2.$m. > $3
dnl # This will cause all names that end in your domain name ($m) to be sent
dnl # via SMTP; anything else will be sent via suucp (smart UUCP) to uunet.
dnl # If you have FEATURE(nocanonify), you may need to omit the dots after
dnl # the $m.
dnl #
dnl # If you are running a local DNS inside your domain which is not
dnl # otherwise connected to the outside world, you probably want to use:
dnl #   define(`SMART_HOST', smtp:fire.wall.com)
dnl #   LOCAL_NET_CONFIG
dnl #   R$* < @ $* . > $*       $#smtp $@ $2. $: $1 < @ $2. > $3
dnl # That is, send directly only to things you found in your DNS lookup;
dnl # anything else goes through SMART_HOST.
