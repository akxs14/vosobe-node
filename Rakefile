require 'rake'
require 'rake/builder'

Rake::Builder.new do |builder|
  builder.target = "vosdemon"

  builder.target_type = :executable
  builder.programming_language = 'c++'
  builder.architecture = "x86_64"

  builder.source_search_paths = ['src']
  builder.objects_path  = 'objects'
  builder.include_paths = ['include']
  builder.library_dependencies = ['msgpack', 'msgpack-rpc', 'mpio', 'virt']
end
