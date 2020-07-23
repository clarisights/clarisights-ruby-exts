# frozen_string_literal: true

require 'mkmf'

ext_name = 'posix_downcase'
dir_config(ext_name)
create_makefile "posix_downcase/posix_downcase"
