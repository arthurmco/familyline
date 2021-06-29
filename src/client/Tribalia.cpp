
static int show_starting_menu();




int main(int argc, char const* argv[])
{
    // Start log as early as possible

    FILE* fLog = nullptr;

    int i = get_arg_index("--log", argc, argv);
    if (i >= 0) {
        if ((i + 1) >= argc) {
            fmt::print(stderr, "--log: expected a filename or 'screen'\n");
            return EXIT_FAILURE;
        }

        fmt::print(stderr, "logging in {:s}\n", argv[i + 1]);
        if (!strcmp(argv[i + 1], "screen")) {
            fLog = stderr;
        } else {
            fLog = fopen(argv[i + 1], "a");
            fputs("\n\n", fLog);
        }
        if (!fLog) {
            perror("Could not create log file: ");
            fmt::print(stderr, "Using defaults");
        }
    }

    if (!fLog) {

        } else {
            errno = 0;  // no need to worry
            /* Tries to create tribalia.log, fallback to stderr if it can't */
            fLog = fopen("familyline.log", "a");
            fputs("\n\n\n\n\n\n\n\n\n\n\n", fLog);
            if (!fLog) {
                perror("Could not create log file: ");
                fmt::print(stderr, "Falling back to stderr");
                fLog = stderr;
            }
        }
    }


    // int winW = 640, winH = 480;
    if (get_arg_index("--version", argc, argv) >= 0) {
        show_version();
        return EXIT_SUCCESS;
    }

    i = get_arg_index("--size", argc, argv);
    if (i >= 0) {
        if (check_size(i, argc, argv) < 0) {
            return EXIT_FAILURE;
        }
    }

    if (get_arg_index("--help", argc, argv) >= 0) {
        show_help();
        return EXIT_SUCCESS;
    }


    return show_starting_menu();
}

static void enable_gl_debug();

static int show_starting_menu()
{

}

