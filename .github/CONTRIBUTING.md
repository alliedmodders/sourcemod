# Contributing to SourceMod

## Issue reports

Please consider the following guidelines when reporting an issue.

#### Not for general support
This is not the right place to get help with using or installing SourceMod, or for issues with specific, third-party SourceMod plugins or extensions.

For help with SourceMod, please consult the [AlliedModders forums](https://forums.alliedmods.net/forumdisplay.php?f=52). Similarly, for assistance with, or to report issues with, third-party SourceMod plugins or extensions, you should post in the existing thread for that plugin or extension on the [AlliedModders forums](https://forums.alliedmods.net/forumdisplay.php?f=52).

#### Details, details, details
Provide as much detail as possible when reporting an issue.

For bugs or other undesired behavior, answers to the following questions are a great start:
* What is the issue?
* What behavior are you expecting instead?
* On what operating system is the game server running?
* What game is the game server running?
* What exact version (full x.y.z.a version number) of Metamod:Source and SourceMod are installed on the game server?
* What is the specific, shortest path to reproducing this issue? If this issue can be reproduced with plugin code, please try to shorten it to the minimum required to trigger the problem.
 
If this is a feature request, the following are helpful. Generally, not all will apply, but whatever you can answer ahead of time will shorten back and forth conversation.
* What is your end goal, or what are you trying to accomplish?
* Why is this necessary, or what benefit do you see with it?
* Will this be useful to others?

#### Issues with security implications
Please report any security bugs to [security@alliedmods.net](mailto:security@alliedmods.net) rather than to this public issue tracker.

#### We're only human
Please keep in mind that we maintain this project in our spare time, at no cost. There is no SLA, and you are not owed a response or a fix.

#### Conduct
Please refer to the [AlliedModders forum rules.](https://forums.alliedmods.net/misc.php?do=showrules)

## Pull Requests

Firstly, thank you for considering contributing changes to the project!

However, if this is anything more than a small fix such as a gamedata update, a glaring code flaw, or a simple typo in a file like this one, please file an issue first so that it can be discussed, unless you have already spoken to multiple members of the development team about it on IRC or the AlliedModders forums.

We don't like to have to reject pull requests, so we want to avoid those scenarios. We wouldn't want you to feel like you wasted your time writing something only for us to shoot it down.

#### Rejection
*Copied from Phabricator's [Contributing Code guidelines](https://secure.phabricator.com/book/phabcontrib/article/contributing_code/#rejecting-patches), as we largely feel the same way about this.*

> If you send us a patch without coordinating it with us first, it will probably be immediately rejected, or sit in limbo for a long time and eventually be rejected. The reasons we do this vary from patch to patch, but some of the most common reasons are:
>
> **Unjustifiable Costs**: We support code in the upstream forever. Support is enormously expensive and takes up a huge amount of our time. The cost to support a change over its lifetime is often 10x or 100x or 1000x greater than the cost to write the first version of it. Many uncoordinated patches we receive are "white elephants", which would cost much more to maintain than the value they provide.
> 
> As an author, it may look like you're giving us free work and we're rejecting it as too expensive, but this viewpoint doesn't align with the reality of a large project which is actively supported by a small, experienced team. Writing code is cheap; maintaining it is expensive.
>
> By coordinating with us first, you can make sure the patch is something we consider valuable enough to put long-term support resources behind, and that you're building it in a way that we're comfortable taking over.
> 
> **Not a Good Fit**: Many patches aren't good fits for the upstream: they implement features we simply don't want. You can find more information in Contributing Feature Requests. Coordinating with us first helps make sure we're on the same page and interested in a feature.
>
> The most common type of patch along these lines is a patch which adds new configuration options. We consider additional configuration options to have an exceptionally high lifetime support cost and are very unlikely to accept them. Coordinate with us first.
> 
> **Not a Priority**: If you send us a patch against something which isn't a priority, we probably won't have time to look at it. We don't give special treatment to low-priority issues just because there's code written: we'd still be spending time on something lower-priority when we could be spending it on something higher-priority instead.
> 
> If you coordinate with us first, you can make sure your patch is in an area of the codebase that we can prioritize.
> 
> **Overly Ambitious Patches**: Sometimes we'll get huge patches from new contributors. These can have a lot of fundamental problems and require a huge amount of our time to review and correct. If you're interested in contributing, you'll have more success if you start small and learn as you go.
> 
> We can help you break a large change into smaller pieces and learn how the codebase works as you proceed through the implementation, but only if you coordinate with us first.
> 
> **Generality**: We often receive several feature requests which ask for similar features, and can come up with a general approach which covers all of the use cases. If you send us a patch for your use case only, the approach may be too specific. When a cleaner and more general approach is available, we usually prefer to pursue it.
> 
> By coordinating with us first, we can make you aware of similar use cases and opportunities to generalize an approach. These changes are often small, but can have a big impact on how useful a piece of code is.
> 
> **Infrastructure and Sequencing**: Sometimes patches are written against a piece of infrastructure with major planned changes. We don't want to accept these because they'll make the infrastructure changes more difficult to implement.
> 
> Coordinate with us first to make sure a change doesn't need to wait on other pieces of infrastructure. We can help you identify technical blockers and possibly guide you through resolving them if you're interested.
