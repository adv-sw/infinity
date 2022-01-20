nsresult Adv_ModalDialog(nsGlobalWindowOuter *gw, const nsAutoString &title, 
                         const nsAutoString &message, nsAString *editable, bool *ok);
						 
bool Gecko_Embed();

nsresult Adv_RequestExclusive(nsGlobalWindowOuter *gw, bool enable);