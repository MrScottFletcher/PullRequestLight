# PullRequestLight
A light indicator for a team's Azure DevOps pull request.  It uses an Arduinio Huzzah Feather, a strip of 77 addressable LEDs glued inside a spiral tower sculpture constructed our of aluminum roof flashing and a 3D-printed base.

Since the network that the device will be on does not allow inbound connections nor peer communication, there are a couple of possible options for "Azure DevOps" (ADO) API access; 1) build an Azure IoT service that manages webhooks from ADO, or 2) do interval polling with an ADO API key.  My initial version will use poling, though a coworker has already demonstrated an IoT proof-of-concept.  p.s. Don't bother looking for project paths, credentials, or API keys in this source code;  They are not in here.

I plan on posting a YouTube walkthrough of the build and some details of the implmentation if/when I complete this project.  I'll post it over at www.youtube.com/c/ScottFletcherTV
