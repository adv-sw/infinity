nsresult Adv_ModalDialog(nsGlobalWindowOuter *gw, const nsAutoString &title, 
                         const nsAutoString &message, nsAString *editable, bool *ok);
						 
bool _Gecko_OffscreenSharedSurfaceMode();

nsresult Adv_RequestExclusive(nsGlobalWindowOuter *gw, bool enable);