$:.push File.expand_path("lib", __dir__)


# Describe your gem and declare its dependencies:
Gem::Specification.new do |spec|
  spec.name        = "posix_downcase"
  spec.version     = '0.0.1'
  spec.authors     = ["Annotation Clarisights"]
  spec.email       = ["annotation@clarisights.com"]
  spec.homepage    = ""
  spec.summary     = "Define string methods for posix case transformation"
  spec.description = spec.summary

  # Prevent pushing this gem to RubyGems.org. To allow pushes either set the 'allowed_push_host'
  # to allow pushing to a single host or delete this section to allow pushing to any host.
  if spec.respond_to?(:metadata)
    spec.metadata["allowed_push_host"] = "TODO: Set to 'http://mygemserver.com'"
  else
    raise "RubyGems 2.0 or newer is required to protect against " \
      "public gem pushes."
  end

  spec.files = Dir["{ext,lib}/**/*", "Rakefile", "README.md"]
  spec.extensions << 'ext/posix_downcase/extconf.rb'

  spec.add_development_dependency 'rake-compiler'
end
