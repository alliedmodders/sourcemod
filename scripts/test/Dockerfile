
#   compiler
#   gcc on debian with multilib
FROM bitnami/minideb:bullseye AS compiler

# We are not an interactive frontend, don't bother us!
ENV DEBIAN_FRONTEND noninteractive

#   gcc
RUN apt update
RUN apt -y install g++ gcc make
RUN apt -y install gcc-multilib 
RUN apt -y install g++-multilib

#   git
RUN apt -y install git

RUN gcc --version
RUN git version --build-options

#   gdc_build
#   Create gdc binaries
FROM compiler AS gdc_build

#   Clone SourceMod
RUN git clone --recursive https://github.com/alliedmodders/sourcemod.git 
WORKDIR sourcemod

#   Checkout stable branch
#RUN git checkout master
#RUN git submodule update --remote 

#   Mkdir to gdc
WORKDIR tools/gdc-psyfork
#   Compile
RUN make

#   depot_downloader_clone
#   Clone depot downloader
FROM bitnami/git as depot_downloader_clone

RUN git clone https://github.com/SteamRE/DepotDownloader.git

#   depot_downloader_build
#   Uses dotnet to build depot downloader cloned from earlier stage
FROM mcr.microsoft.com/dotnet/sdk:6.0-alpine as depot_downloader_build

COPY --from=depot_downloader_clone DepotDownloader/DepotDownloader DepotDownloader
WORKDIR DepotDownloader

RUN dotnet restore
RUN dotnet build -c Release --no-restore

#   runtime
#   contains gdc, depot downloader,
FROM mcr.microsoft.com/dotnet/runtime:6.0-alpine AS runtime

#   depot downloader
COPY --from=depot_downloader_build DepotDownloader/bin/Release/net6.0 tools/depotdownloader
ENV DEPOT_DOWNLOADER_PATH="tools/depotdownloader/DepotDownloader.dll"

#   depot download
#   download the server binaries!
FROM runtime as depot_downloader

#   Filelist
#   Otherwise we're downloading 2gigs of junk :(
COPY scripts/test/filelist.txt filelist.txt

#   depot version
#   used to bust the cache and prevent stale versions from being gamedata-checked.
ARG DEPOT_VERSION
ARG DEPOT_ID
ARG APP_ID

#   Fetch CS:GO
RUN dotnet $DEPOT_DOWNLOADER_PATH -app $APP_ID -validate -all-platforms -dir server -filelist filelist.txt

  
#   runtime_multiarch
#   Creates a multi-architecture distro that can run gdb on x64 hosts
#   Use bullseye to be compat with ubuntu focal glibc
FROM bitnami/minideb:bullseye as runtime_multiarch

ENV DEBIAN_FRONTEND noninteractive

RUN dpkg --add-architecture i386
RUN apt-get update

RUN apt-get -y install libc6:i386 
RUN apt-get -y install libstdc++6:i386

FROM runtime_multiarch AS test_runtime


WORKDIR /test

#   gdc
COPY --from=gdc_build sourcemod/tools/gdc-psyfork/Release tools/gdc
COPY --from=gdc_build sourcemod/tools/gdc-psyfork/symbols.txt tools/gdc/symbols.txt

#   server binaries
COPY --from=depot_downloader server server
COPY gamedata gamedata

#   Add depot libs to search path'
ENV LD_LIBRARY_PATH /test/server:/test/server/bin:$LD_LIBRARY_PATH

WORKDIR /test/server
ENTRYPOINT [ "../tools/gdc/gdc" ]
