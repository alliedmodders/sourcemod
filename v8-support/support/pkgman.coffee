# Quick and dirty dependency manager
# Version and Requirement mainly copied 
# from rubygems and converted to coffeescript

# the pakfile will be executed with an empty object as this.
@parsePakfile = (pakfile,dep) ->
	(->
		eval(pakfile)
	).call({})

# We do NOT want pakfiles to be able to mess with the dependency manager code
# Do not place critical vars outside this guard since it *will* be accessible by pakfiles
(->
	class Version
		constructor: (ver) ->
			@version = ver.toString().trim()

		Version.isCorrect = (version) ->
			/^[0-9]+(\.[0-9a-zA-Z]+)*$/.test(version.toString().trim())

		Version.newestOfVersions = (versions) ->
			return null if versions.length == 0
			versions.reduce((a,b) -> if a.compareTo(b) >= 0 then a else b)

		hasPrereleaseSegments = (segments) ->
			segments.some((segment) ->	typeof(segment) == "string")

		bump: ->
			segments = @segments().slice()
			segments.pop() while hasPrereleaseSegments(segments)
			segments.pop() if segments.length > 1

			segments[segments.length-1]++
			new Version(segments.join("."))

		isEqual: (other) ->
			@version == other.version

		isPrerelease: ->
			/[a-zA-Z]/.test(@version)

		release: ->
			return @ unless @isPrerelease()

			segments = @segments().slice()
			segments.pop() while hasPrereleaseSegments(segments)
			new Version(segments.join("."))

		segments: ->
			(if isNaN(parseInt(segment)) then segment else parseInt(segment)) for segment in @version.match(/[0-9]+|[a-z]+/ig)

		approximateRecommendation: ->
			segments = @segments().slice()

			segments.pop()   while hasPrereleaseSegments(segments)
			segments.pop()   while segments.length > 2
			segments.push(0) while segments.length < 2

			"~> #{segments.join(".")}"

		compareTo: (other) ->
			return 0 if @version == other.version

			lhsegments = @segments()
			rhsegments = other.segments()

			lhsize = lhsegments.length
			rhsize = rhsegments.length
			limit = Math.max(lhsize,rhsize) - 1

			i = 0
			while i <= limit
				[lhs, rhs] = [lhsegments[i] ? 0, rhsegments[i] ? 0]
				i += 1

				continue  if lhs == rhs
				return -1 if typeof(lhs) == "string" && typeof(rhs) == "number"
				return  1 if typeof(lhs) == "number" && typeof(rhs) == "string"

				return (if lhs < rhs then -1 else 1)

			return 0

	Array::unique = ->
		output = {}
		output[@[key]] = @[key] for key in [0...@length]
		value for key, value of output
	Array::dropNull = ->
		value for value in @ when value?

	class Requirement
		OPS =
			"=": (v,r) ->
				v.compareTo(r) == 0
			"!=": (v,r) ->
				v.compareTo(r) != 0
			">": (v,r) ->
				v.compareTo(r) > 0
			"<": (v,r) ->
				v.compareTo(r) < 0
			">=": (v,r) ->
				v.compareTo(r) >= 0
			"<=": (v,r) ->
				v.compareTo(r) <= 0
			"~>": (v,r) ->
				v.compareTo(r) >= 0 && v.release().compareTo(r.bump()) < 0

		DefaultRequirement = [">=", new Version(0)]

		Requirement.parse = (obj) ->
			return ["=", obj] if typeof(obj) == "object" && Version:: == Object.getPrototypeOf(obj)

			matched = /^(=|!=|>|<|>=|<=|~>)?\s*([0-9]+(?:\.[0-9a-zA-Z]+)*)$/.exec(obj.trim())
			throw "Illformed requirement: #{obj}" unless matched?

			if matched[1] == ">=" && matched[2] == "0"
				DefaultRequirement
			else
				[matched[1] ? "=", new Version(matched[2])]

		# difference: no flatten
		constructor: (requirements) ->
			requirements = requirements.dropNull()
			requirements = requirements.unique()

			if requirements.length == 0
				@requirements = [DefaultRequirement]
			else
				@requirements = (Requirement.parse(r) for r in requirements)

		# TODO: potential bug
		hasNone: ->
			if @requirements.length == 1
				@requirements[0] == DefaultRequirement
			else
				false

		isPrerelease: ->
			@requirements.some((r) ->
				r[1].isPrerelease())

		isSatisfiedBy: (version) ->
			throw "Needs a Version" unless typeof(version) == "object" && Version:: == Object.getPrototypeOf(version)
			@requirements.every((r) ->
				[op, rv] = r
				(OPS[op] ? OPS["="])(version,rv))

		isSpecific: ->
			return true if @requirements.length > 1

			[">",">="].indexOf(@requirements[0][0]) == -1


	class PkgProvider
		constructor: (source, pkg, versions) ->
			@source = source
			@pkg = pkg
			@versions = (new Version(version) for version in versions)

		newestVersion: () ->
			Version.newestOfVersions(@versions)

		bestSatisfier: (requirement) ->
			satisfyingVersions = (version for version in @versions when !version.isPrerelease() && requirement.isSatisfiedBy(version))
			Version.newestOfVersions(satisfyingVersions)

		compareTo: (other, requirement) ->
			throw "compareTo requires PkgProvider" unless PkgProvider::isPrototypeOf(other)
			throw "compareTo requires provider of same pkg" unless other.pkg == @pkg

			mySatisfier = @bestSatisfier(requirement)
			otherSatisfier = other.bestSatisfier(requirement)

			mySatisfier.compareTo(otherSatisfier)

		installBestSatisfier: (requirement) ->
			version = @bestSatisfier(requirement)
			@source.install(@pkg,version.version)
			"#{@pkg}/#{version.version}"

		PkgProvider.bestOfProviders = (providers, requirement) ->
			return null if providers.length == 0
			providers.reduce((a,b) -> if a.compareTo(b,requirement) >= 0 then a else b)

	class LocalSource
		lookupPkg: (pkg) ->
			versions = externals.findLocalVersions(pkg)
			new PkgProvider(this, pkg, versions)

		install: (pkg, version) ->
			true

	(->
		remoteSources = []
		localSource = new LocalSource()
		pkgAliases = {}
		allowUpgradeOnTheFly = false
		currentPkg = null
		global = @

		findLocalProvider = (pkg, requirement) ->
			localProvider = localSource.lookupPkg(pkg)
			if localProvider? && localProvider.bestSatisfier(requirement)? then localProvider else null

		findRemoteProvider = (pkg, requirement) ->
			remoteProviders = (source.lookupPkg(pkg) for source in remoteSources)
			remoteProviders = remoteProviders.dropNull()
			remoteProviders = (remoteProvider for remoteProvider in remoteProviders when remoteProvider.bestSatisfier(requirement)?)

			PkgProvider.bestOfProviders(remoteProviders, requirement)

		pakfileExposed = 
			dep: (pkg, requirements...) ->
				requirement = new Requirement(requirements)
				rSat = findRemoteProvider(pkg, requirement)
				lSat = findLocalProvider(pkg, requirement)
				providers = [lSat]

				if allowUpgradeOnTheFly
					providers.push(rSat)
				else
					log("A new version is available for package #{pkg}: #{rSat.bestSatisfier(requirement).version}") if rSat? && lSat? && PkgProvider.bestOfProviders(rSat.compareTo(lSat,requirement) > 0)

				providers = providers.dropNull()

				throw "Can't find source for package #{pkg}! Aborting..." if providers.length == 0

				provider = PkgProvider.bestOfProviders(providers,requirement)
				pkgAliases[currentPkg][pkg] = provider.installBestSatisfier(requirement)

		addRemoteSource = (source) ->
			remoteSources.push(source)

		loadDependencies = (pkg) ->
			currentPkg = pkg

			return if pkgAliases[currentPkg]?

			pakfile = externals.readPakfile(currentPkg)

			pkgAliases[currentPkg] = {}
			pkgAliases[currentPkg][pkg.split("/")[0]] = pkg

			# Call the sandboxed pakfile environment, pass our public functions
			global.parsePakfile(pakfile,pakfileExposed.dep)

			subpkgs = (subpkgdir for own subpkg, subpkgdir of pkgAliases[currentPkg])
			loadDependencies(subpkg) for subpkg in subpkgs

		getPkgAliases = (pkg) ->
			pkgAliases[pkg]

		resolvePath = (pkg, path) ->
			firstSlash = path.indexOf("/")
			firstSlash = path.length if firstSlash == -1
			alias = path.substring(0, firstSlash)
			getPkgAliases(pkg)[alias] + path.substring(firstSlash)

		depend = (depender, pkg, requirement) ->
			currentPkg = depender
			pkgAliases[currentPkg] = {}

			pakfileExposed.dep(pkg,requirement)

			subpkgs = (subpkgdir for own subpkg, subpkgdir of pkgAliases[currentPkg])
			loadDependencies(subpkg) for subpkg in subpkgs

		resetAliases = (depender) ->
			pkgAliases[depender] = {}

		@dependencyManager =
			addRemoteSource: addRemoteSource
			loadDependencies: loadDependencies
			getPkgAliases: getPkgAliases
			resolvePath: resolvePath
			depend: depend
			resetAliases: resetAliases
	).call(@)
).call(@)
