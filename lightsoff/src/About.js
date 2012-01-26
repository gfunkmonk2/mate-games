Gtk = imports.gi.Gtk;
MateGamesSupport = imports.gi.MateGamesSupport;

main = imports.main;

_ = imports.gettext.gettext;

function show_about_dialog()
{
	var about_dialog = new Gtk.AboutDialog();
	about_dialog.program_name = _("Lights Off");
	about_dialog.version = "1.0";
	about_dialog.comments = _("Turn off all the lights\n\nLights Off is a part of MATE Games.");
	about_dialog.copyright = _("Copyright \xa9 2009 Tim Horton");
	about_dialog.license = MateGamesSupport.get_license(_("Lights Off"));
	about_dialog.wrap_license = true;
	about_dialog.logo_icon_name = "mate-lightsoff";
	about_dialog.website = "http://mate-desktop.org/";
	about_dialog.website_label = _("MATE Desktop web site");
	about_dialog.translator_credits = _("translator-credits");

	about_dialog.set_authors(["Tim Horton"]);
	about_dialog.set_artists(["Tim Horton", "Ulisse Perusin"]);

	about_dialog.set_transient_for(main.window);
	about_dialog.run();
	
	about_dialog.hide();
}
