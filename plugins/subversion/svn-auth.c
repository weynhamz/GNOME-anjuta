/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
 * svn-backend.c (c) 2005 Naba kumar
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <svn_auth.h>
#include <svn_pools.h>
#include "svn-backend-priv.h"
#include "svn-auth.h"
#include "plugin.h"

#include <libanjuta/anjuta-debug.h>

/* User authentication prompts handlers */
static svn_error_t*
svn_auth_simple_prompt_func_cb (svn_auth_cred_simple_t **cred, void *baton,
								const char *realm, const char *username,
								svn_boolean_t may_save, apr_pool_t *pool)
{
	/* Prompt the user for username && password.
	 * No need to be thread safe at this point as we are in the main thread.
	 * The svn thread will start later!
	 * 
	 * We must prompt the user at this point, pre-filling the username field
	 * with the passed 'username' string. The dialog will have 'Remeber
	 * password' checkbox, which should be insensitive if the passed 'may_save'
	 * value is FALSE.
	 */ 
	
	/* SVN *svn = (SVN*)baton; */
	GladeXML* gxml = glade_xml_new(GLADE_FILE, "svn_user_auth", NULL);
	GtkWidget* svn_user_auth = glade_xml_get_widget(gxml, "svn_user_auth");
	GtkWidget* auth_realm = glade_xml_get_widget(gxml, "auth_realm");
	GtkWidget* username_entry = glade_xml_get_widget(gxml, "username_entry");
	GtkWidget* password_entry = glade_xml_get_widget(gxml, "password_entry");
	GtkWidget* remember_pwd = glade_xml_get_widget(gxml, "remember_pwd");
	svn_error_t *err = NULL;
	
	gtk_dialog_set_default_response (GTK_DIALOG (svn_user_auth), GTK_RESPONSE_OK);
	
	if (realm)
		gtk_label_set_text (GTK_LABEL (auth_realm), realm);
	if (username)
	{
		gtk_entry_set_text (GTK_ENTRY (username_entry), username);
		gtk_widget_grab_focus (password_entry);
	}
	if (!may_save)
		gtk_widget_set_sensitive(remember_pwd, FALSE);
		
	/* Then the dialog is prompted to user and when user clicks ok, the
	 * values entered, i.e username, password and remember password (true
	 * by default) should be used to initialized the memebers below. If the
	 * user cancels the dialog, I think we return an error struct
	 * appropriately initialized. -- naba
	 */
	 
 	switch (gtk_dialog_run(GTK_DIALOG(svn_user_auth)))
	{
		case GTK_RESPONSE_OK:
		{
			*cred = apr_pcalloc (pool, sizeof(*cred));
			(*cred)->may_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
															 (remember_pwd));
			(*cred)->username = apr_pstrdup (pool,
								 gtk_entry_get_text(GTK_ENTRY(username_entry)));
			(*cred)->password = apr_pstrdup (pool,
								 gtk_entry_get_text(GTK_ENTRY(password_entry)));
															 			
			err = SVN_NO_ERROR;
			break;
		}
		default:
			err = svn_error_create (SVN_ERR_AUTHN_CREDS_UNAVAILABLE, NULL,
									_("Authentication canceled"));
									 
			break;
	}
	gtk_widget_destroy (svn_user_auth);
	return err;
}

#if 0
static svn_error_t*
svn_auth_username_prompt_func_cb (svn_auth_cred_username_t **cred,
								  void *baton, const char *realm,
								  svn_boolean_t may_save, apr_pool_t *pool)
{
	/* SVN *svn = (SVN*)baton; */
	GladeXML* gxml = glade_xml_new(GLADE_FILE, "svn_user_auth", NULL);
	GtkWidget* svn_user_auth = glade_xml_get_widget(gxml, "svn_user_auth");
	GtkWidget* auth_realm = glade_xml_get_widget(gxml, "auth_realm");
	GtkWidget* username_entry = glade_xml_get_widget(gxml, "username_entry");
	GtkWidget* password_entry = glade_xml_get_widget(gxml, "password_entry");
	GtkWidget* remember_pwd = glade_xml_get_widget(gxml, "remember_pwd");
	svn_error_t *err = NULL;
	
	gtk_widget_set_sensitive(password_entry, FALSE);
	
	gtk_dialog_set_default_response (GTK_DIALOG (svn_user_auth), GTK_RESPONSE_OK);
	
	if (realm)
		gtk_label_set_text (GTK_LABEL (auth_realm), realm);
	if (!may_save)
		gtk_widget_set_sensitive(remember_pwd, FALSE);
		
	/* Then the dialog is prompted to user and when user clicks ok, the
	 * values entered, i.e username, password and remember password (true
	 * by default) should be used to initialized the memebers below. If the
	 * user cancels the dialog, I think we return an error struct
	 * appropriately initialized. -- naba
	 */
	 
 	switch (gtk_dialog_run(GTK_DIALOG(svn_user_auth)))
	{
		case GTK_RESPONSE_OK:
			*cred = apr_pcalloc (pool, sizeof(*cred));
			(*cred)->username = apr_pstrdup (pool,
								 gtk_entry_get_text(GTK_ENTRY(username_entry)));
			(*cred)->may_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
															 (remember_pwd));
			break;
		default:
			err = svn_error_create (SVN_ERR_AUTHN_CREDS_UNAVAILABLE, NULL,
									_("Authentication canceled"));
									 
			break;
	}
	gtk_widget_destroy (svn_user_auth);
	return err;
}
#endif


static svn_error_t*
svn_auth_ssl_server_trust_prompt_func_cb (svn_auth_cred_ssl_server_trust_t **cred,
										  void *baton, const char *realm,
										  apr_uint32_t failures,
										  const svn_auth_ssl_server_cert_info_t *cert_info,
										  svn_boolean_t may_save,
										  apr_pool_t *pool)
{
	/* SVN *svn = (SVN*)baton; */
	
	GladeXML* gxml = glade_xml_new(GLADE_FILE, "svn_server_trust", NULL);
	GtkWidget* svn_server_trust = glade_xml_get_widget(gxml, "svn_server_trust");
	GtkWidget* auth_realm = glade_xml_get_widget(gxml, "realm_label");
	GtkWidget* server_info = glade_xml_get_widget(gxml, "server_info_label");
	GtkWidget* remember_check = glade_xml_get_widget(gxml, "remember_check");
	svn_error_t *err = NULL;
	gchar* info;
	
	if (realm)
		gtk_label_set_text (GTK_LABEL (auth_realm), realm);
	
	info = g_strconcat(_("Hostname: "), cert_info->hostname, "\n",
					   _("Fingerprint: "), cert_info->fingerprint, "\n",
					   _("Valid from: "), cert_info->valid_from, "\n",
					   _("Valid until: "), cert_info->valid_until, "\n",
					   _("Issuer DN: "), cert_info->issuer_dname, "\n",
					   _("DER certificate: "), cert_info->ascii_cert, "\n",
					   NULL);
	gtk_label_set_text (GTK_LABEL (server_info), info);
	
	if (!may_save)
		gtk_widget_set_sensitive(remember_check, FALSE);
	
	gtk_dialog_set_default_response (GTK_DIALOG (svn_server_trust), GTK_RESPONSE_YES);

	
	switch (gtk_dialog_run(GTK_DIALOG(svn_server_trust)))
	{
		case GTK_RESPONSE_YES:
			*cred = apr_pcalloc (pool, sizeof(*cred));
			(*cred)->may_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
															 (remember_check));
			err = SVN_NO_ERROR;
		/* TODO: Set bitmask for accepted_failures */
			break;
		default:
			err = svn_error_create (SVN_ERR_AUTHN_CREDS_UNAVAILABLE, NULL,
									_("Authentication canceled"));									 
			break;
	}
	gtk_widget_destroy (svn_server_trust);
	return err;
}

static svn_error_t*
svn_auth_ssl_client_cert_prompt_func_cb (svn_auth_cred_ssl_client_cert_t **cred,
										 void *baton, const char *realm,
										 svn_boolean_t may_save,
										 apr_pool_t *pool)
{
	// SVN *svn = (SVN*)baton;
	
	/* Ask for the file where client certificate of authenticity is.
	 * I think it is some sort of private key. */
	return SVN_NO_ERROR;
}

static svn_error_t*
svn_auth_ssl_client_cert_pw_prompt_func_cb (svn_auth_cred_ssl_client_cert_pw_t **cred,
										   void *baton, const char *realm,
										   svn_boolean_t may_save,
										   apr_pool_t *pool)
{
	// SVN *svn = (SVN*)baton;
	
	/* Prompt for password only. I think it is pass-phrase of the above key. */
	return SVN_NO_ERROR;;
}

void
svn_fill_auth(SVN* svn)
{
	svn_auth_baton_t *auth_baton;

    apr_array_header_t *providers
      = apr_array_make (svn->pool, 1, sizeof (svn_auth_provider_object_t *));

    svn_auth_provider_object_t *provider;
	
	/* Provider that authenticates username/password from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_simple_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;

	/* Provider that authenticates server trust from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_server_trust_file_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates client cert from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_file_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates client cert password from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_pw_file_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for username/password */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_simple_prompt_provider(&provider,
										  svn_auth_simple_prompt_func_cb,
										  svn, 3, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;

	/* Provider that prompts for server trust */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_server_trust_prompt_provider (&provider,
								  svn_auth_ssl_server_trust_prompt_func_cb,
													svn, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for client certificate file */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_prompt_provider (&provider,
								  svn_auth_ssl_client_cert_prompt_func_cb,
													svn, 3, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for client certificate file password */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_pw_prompt_provider (&provider,
								  svn_auth_ssl_client_cert_pw_prompt_func_cb,
													svn, 3, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
    svn_auth_open (&auth_baton, providers, svn->pool);

    svn->ctx->auth_baton = auth_baton;
}
