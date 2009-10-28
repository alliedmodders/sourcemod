

public OnPluginStart()
{
	RegServerCmd("sm_test_caps1", Test_Caps1);
	RegServerCmd("sm_test_caps2", Test_Caps2);
	RegServerCmd("sm_test_caps3", Test_Caps3);
}

public Action:Test_Caps1(args)
{
	PrintToServer("CanTestFeatures: %d", CanTestFeatures());
	PrintToServer("Status PTS: %d", GetFeatureStatus(FeatureType_Native, "PrintToServer"));
	PrintToServer("Status ???: %d", GetFeatureStatus(FeatureType_Native, "???"));
	PrintToServer("Status CL: %d", GetFeatureStatus(FeatureType_Capability, FEATURECAP_COMMANDLISTENER));

	return Plugin_Handled
}

public Action:Test_Caps2(args)
{
	RequireFeature(FeatureType_Native, "VerifyCoreVersion");
	RequireFeature(FeatureType_Native, "Sally ate a worm");
}

public Action:Test_Caps3(args)
{
	RequireFeature(FeatureType_Native, "Sally ate a worm", "oh %s %d no", "yam", 23);
}




