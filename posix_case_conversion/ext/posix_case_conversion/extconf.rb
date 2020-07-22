# frozen_string_literal: true

require 'mkmf'

ext_name = 'posix_case_conversion'
dir_config(ext_name)
create_makefile "posix_case_conversion/posix_case_conversion"
